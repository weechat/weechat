/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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

/*
 * relay-weechat-protocol.c: WeeChat protocol for relay to client
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../../weechat-plugin.h"
#include "../relay.h"
#include "relay-weechat.h"
#include "relay-weechat-protocol.h"
#include "relay-weechat-msg.h"
#include "../relay-client.h"
#include "../relay-config.h"
#include "../relay-raw.h"


/*
 * relay_weechat_protocol_get_buffer: get buffer pointer with argument from a
 *                                    command, which can be a pointer
 *                                    ("0x12345") or a full name
 *                                    ("irc.freenode.#weechat")
 */

struct t_gui_buffer *
relay_weechat_protocol_get_buffer (const char *arg)
{
    struct t_gui_buffer *ptr_buffer;
    long unsigned int value;
    int rc;
    char *pos, *plugin;
    struct t_hdata *ptr_hdata;

    ptr_buffer = NULL;

    if (strncmp (arg, "0x", 2) == 0)
    {
        rc = sscanf (arg, "%lx", &value);
        if ((rc != EOF) && (rc != 0))
            ptr_buffer = (struct t_gui_buffer *)value;
        if (ptr_buffer)
        {
            ptr_hdata = weechat_hdata_get ("buffer");
            if (!weechat_hdata_check_pointer (ptr_hdata,
                                              weechat_hdata_get_list (ptr_hdata, "gui_buffers"),
                                              ptr_buffer))
            {
                /* invalid pointer! */
                ptr_buffer = NULL;
            }
        }
    }
    else
    {
        pos = strchr (arg, '.');
        if (pos)
        {
            plugin = weechat_strndup (arg, pos - arg);
            if (plugin)
            {
                ptr_buffer = weechat_buffer_search (plugin, pos + 1);
                free (plugin);
            }
        }
    }

    return ptr_buffer;
}

/*
 * relay_weechat_protocol_cb_init: 'init' command from client
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(init)
{
    char **options, *pos;
    int num_options, i, compression;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(1);

    options = weechat_string_split (argv_eol[0], ",", 0, 0, &num_options);
    if (options)
    {
        for (i = 0; i < num_options; i++)
        {
            pos = strchr (options[i], '=');
            if (pos)
            {
                pos[0] = '\0';
                pos++;
                if (strcmp (options[i], "password") == 0)
                {
                    if (strcmp (weechat_config_string (relay_config_network_password),
                                pos) == 0)
                    {
                        RELAY_WEECHAT_DATA(client, password_ok) = 1;
                    }
                }
                else if (strcmp (options[i], "compression") == 0)
                {
                    compression = relay_weechat_compression_search (pos);
                    if (compression >= 0)
                        RELAY_WEECHAT_DATA(client, compression) = compression;
                }
            }
        }
        weechat_string_free_split (options);
    }

    return WEECHAT_RC_OK;
}

/*
 * relay_weechat_protocol_cb_hdata: 'hdata' command from client
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(hdata)
{
    struct t_relay_weechat_msg *msg;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(1);

    msg = relay_weechat_msg_new (id);
    if (msg)
    {
        relay_weechat_msg_add_hdata (msg, argv[0],
                                     (argc > 1) ? argv_eol[1] : NULL);
        relay_weechat_msg_send (client, msg, 1);
        relay_weechat_msg_free (msg);
    }

    return WEECHAT_RC_OK;
}

/*
 * relay_weechat_protocol_cb_info: 'info' command from client
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(info)
{
    struct t_relay_weechat_msg *msg;
    const char *info;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(1);

    msg = relay_weechat_msg_new (id);
    if (msg)
    {
        info = weechat_info_get (argv[0],
                                 (argc > 1) ? argv_eol[1] : NULL);
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_INFO);
        relay_weechat_msg_add_string (msg, argv[0]);
        relay_weechat_msg_add_string (msg, info);
        relay_weechat_msg_send (client, msg, 1);
        relay_weechat_msg_free (msg);
    }

    return WEECHAT_RC_OK;
}

/*
 * relay_weechat_protocol_cb_infolist: 'infolist' command from client
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(infolist)
{
    struct t_relay_weechat_msg *msg;
    long unsigned int value;
    char *args;
    int rc;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(1);

    msg = relay_weechat_msg_new (id);
    if (msg)
    {
        value = 0;
        args = NULL;
        if (argc > 1)
        {
            rc = sscanf (argv[1], "%lx", &value);
            if ((rc == EOF) || (rc == 0))
                value = 0;
            if (argc > 2)
                args = argv_eol[2];
        }
        relay_weechat_msg_add_infolist (msg, argv[0], (void *)value, args);
        relay_weechat_msg_send (client, msg, 1);
        relay_weechat_msg_free (msg);
    }

    return WEECHAT_RC_OK;
}

/*
 * relay_weechat_protocol_cb_nicklist: 'nicklist' command from client
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(nicklist)
{
    struct t_relay_weechat_msg *msg;
    struct t_gui_buffer *ptr_buffer;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(0);

    ptr_buffer = NULL;

    if (argc > 0)
    {
        ptr_buffer = relay_weechat_protocol_get_buffer (argv[0]);
        if (!ptr_buffer)
            return WEECHAT_RC_OK;
    }

    msg = relay_weechat_msg_new (id);
    if (msg)
    {
        relay_weechat_msg_add_nicklist (msg, ptr_buffer);
        relay_weechat_msg_send (client, msg, 1);
        relay_weechat_msg_free (msg);
    }

    return WEECHAT_RC_OK;
}

/*
 * relay_weechat_protocol_cb_input: 'input' command from client
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(input)
{
    struct t_gui_buffer *ptr_buffer;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(2);

    ptr_buffer = relay_weechat_protocol_get_buffer (argv[0]);
    if (ptr_buffer)
        weechat_command (ptr_buffer, argv_eol[1]);

    return WEECHAT_RC_OK;
}

/*
 * relay_weechat_protocol_signal_buffer_cb: callback for "buffer_*" signals
 */

int
relay_weechat_protocol_signal_buffer_cb (void *data, const char *signal,
                                         const char *type_data,
                                         void *signal_data)
{
    struct t_relay_client *ptr_client;
    struct t_gui_line *ptr_line;
    struct t_hdata *ptr_hdata_line, *ptr_hdata_line_data;
    struct t_gui_line_data *ptr_line_data;
    struct t_gui_buffer *ptr_buffer;
    struct t_relay_weechat_msg *msg;
    char cmd_hdata[64], str_signal[128];

    /* make C compiler happy */
    (void) type_data;

    ptr_client = (struct t_relay_client *)data;
    if (!ptr_client || !relay_client_valid (ptr_client))
        return WEECHAT_RC_OK;

    snprintf (str_signal, sizeof (str_signal), "_%s", signal);

    if (strcmp (signal, "buffer_opened") == 0)
    {
        ptr_buffer = (struct t_gui_buffer *)signal_data;
        if (!ptr_buffer)
            return WEECHAT_RC_OK;

        msg = relay_weechat_msg_new (str_signal);
        if (msg)
        {
            snprintf (cmd_hdata, sizeof (cmd_hdata),
                      "buffer:0x%lx", (long unsigned int)ptr_buffer);
            relay_weechat_msg_add_hdata (msg, cmd_hdata,
                                         "number,full_name,short_name,"
                                         "nicklist,title,local_variables,"
                                         "prev_buffer,next_buffer");
            relay_weechat_msg_send (ptr_client, msg, 0);
            relay_weechat_msg_free (msg);
        }
    }
    else if (strcmp (signal, "buffer_type_changed") == 0)
    {
        ptr_buffer = (struct t_gui_buffer *)signal_data;
        if (!ptr_buffer)
            return WEECHAT_RC_OK;

        msg = relay_weechat_msg_new (str_signal);
        if (msg)
        {
            snprintf (cmd_hdata, sizeof (cmd_hdata),
                      "buffer:0x%lx", (long unsigned int)ptr_buffer);
            relay_weechat_msg_add_hdata (msg, cmd_hdata,
                                         "number,full_name,type");
            relay_weechat_msg_send (ptr_client, msg, 0);
            relay_weechat_msg_free (msg);
        }
    }
    else if (strcmp (signal, "buffer_moved") == 0)
    {
        ptr_buffer = (struct t_gui_buffer *)signal_data;
        if (!ptr_buffer)
            return WEECHAT_RC_OK;

        msg = relay_weechat_msg_new (str_signal);
        if (msg)
        {
            snprintf (cmd_hdata, sizeof (cmd_hdata),
                      "buffer:0x%lx", (long unsigned int)ptr_buffer);
            relay_weechat_msg_add_hdata (msg, cmd_hdata,
                                         "number,full_name,"
                                         "prev_buffer,next_buffer");
            relay_weechat_msg_send (ptr_client, msg, 0);
            relay_weechat_msg_free (msg);
        }
    }
    else if ((strcmp (signal, "buffer_merged") == 0)
             || (strcmp (signal, "buffer_unmerged") == 0))
    {
        ptr_buffer = (struct t_gui_buffer *)signal_data;
        if (!ptr_buffer)
            return WEECHAT_RC_OK;

        msg = relay_weechat_msg_new (str_signal);
        if (msg)
        {
            snprintf (cmd_hdata, sizeof (cmd_hdata),
                      "buffer:0x%lx", (long unsigned int)ptr_buffer);
            relay_weechat_msg_add_hdata (msg, cmd_hdata,
                                         "number,full_name,"
                                         "prev_buffer,next_buffer");
            relay_weechat_msg_send (ptr_client, msg, 0);
            relay_weechat_msg_free (msg);
        }
    }
    else if (strcmp (signal, "buffer_renamed") == 0)
    {
        ptr_buffer = (struct t_gui_buffer *)signal_data;
        if (!ptr_buffer)
            return WEECHAT_RC_OK;

        msg = relay_weechat_msg_new (str_signal);
        if (msg)
        {
            snprintf (cmd_hdata, sizeof (cmd_hdata),
                      "buffer:0x%lx", (long unsigned int)ptr_buffer);
            relay_weechat_msg_add_hdata (msg, cmd_hdata,
                                         "number,full_name,short_name,"
                                         "local_variables");
            relay_weechat_msg_send (ptr_client, msg, 0);
            relay_weechat_msg_free (msg);
        }
    }
    else if (strcmp (signal, "buffer_title_changed") == 0)
    {
        ptr_buffer = (struct t_gui_buffer *)signal_data;
        if (!ptr_buffer)
            return WEECHAT_RC_OK;

        msg = relay_weechat_msg_new (str_signal);
        if (msg)
        {
            snprintf (cmd_hdata, sizeof (cmd_hdata),
                      "buffer:0x%lx", (long unsigned int)ptr_buffer);
            relay_weechat_msg_add_hdata (msg, cmd_hdata,
                                         "number,full_name,title");
            relay_weechat_msg_send (ptr_client, msg, 0);
            relay_weechat_msg_free (msg);
        }
    }
    else if (strncmp (signal, "buffer_localvar_", 16) == 0)
    {
        ptr_buffer = (struct t_gui_buffer *)signal_data;
        if (!ptr_buffer)
            return WEECHAT_RC_OK;

        msg = relay_weechat_msg_new (str_signal);
        if (msg)
        {
            snprintf (cmd_hdata, sizeof (cmd_hdata),
                      "buffer:0x%lx", (long unsigned int)ptr_buffer);
            relay_weechat_msg_add_hdata (msg, cmd_hdata,
                                         "number,full_name,local_variables");
            relay_weechat_msg_send (ptr_client, msg, 0);
            relay_weechat_msg_free (msg);
        }
    }
    else if (strcmp (signal, "buffer_line_added") == 0)
    {
        ptr_line = (struct t_gui_line *)signal_data;
        if (!ptr_line)
            return WEECHAT_RC_OK;

        ptr_hdata_line = weechat_hdata_get ("line");
        if (!ptr_hdata_line)
            return WEECHAT_RC_OK;

        ptr_hdata_line_data = weechat_hdata_get ("line_data");
        if (!ptr_hdata_line_data)
            return WEECHAT_RC_OK;

        ptr_line_data = weechat_hdata_pointer (ptr_hdata_line, ptr_line, "data");
        if (!ptr_line_data)
            return WEECHAT_RC_OK;

        ptr_buffer = weechat_hdata_pointer (ptr_hdata_line_data, ptr_line_data,
                                            "buffer");
        if (!ptr_buffer)
            return WEECHAT_RC_OK;

        /* check if buffer is synchronized (== able to receive events) */
        if (weechat_hashtable_has_key (RELAY_WEECHAT_DATA(ptr_client, buffers_sync),
                                       "*")
            || weechat_hashtable_has_key (RELAY_WEECHAT_DATA(ptr_client, buffers_sync),
                                          weechat_buffer_get_string (ptr_buffer,
                                                                     "full_name")))
        {
            msg = relay_weechat_msg_new (str_signal);
            if (msg)
            {
                snprintf (cmd_hdata, sizeof (cmd_hdata),
                          "line_data:0x%lx",
                          (long unsigned int)ptr_line_data);
                relay_weechat_msg_add_hdata (msg, cmd_hdata,
                                             "buffer,date,date_printed,"
                                             "displayed,highlight,prefix,"
                                             "message");
                relay_weechat_msg_send (ptr_client, msg, 0);
                relay_weechat_msg_free (msg);
            }
        }
    }
    else if (strcmp (signal, "buffer_closing") == 0)
    {
        ptr_buffer = (struct t_gui_buffer *)signal_data;
        if (!ptr_buffer)
            return WEECHAT_RC_OK;

        msg = relay_weechat_msg_new (str_signal);
        if (msg)
        {
            snprintf (cmd_hdata, sizeof (cmd_hdata),
                      "buffer:0x%lx", (long unsigned int)ptr_buffer);
            weechat_hashtable_remove (RELAY_WEECHAT_DATA(ptr_client, buffers_nicklist),
                                      cmd_hdata + 7);
            relay_weechat_msg_add_hdata (msg, cmd_hdata,
                                         "number,full_name");
            relay_weechat_msg_send (ptr_client, msg, 0);
            relay_weechat_msg_free (msg);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * relay_weechat_protocol_nicklist_map_cb: callback for entries in hashtable
 *                                         "buffers_nicklist" of client
 *                                         (send nicklist for each buffer in
 *                                         this hashtable)
 */

void
relay_weechat_protocol_nicklist_map_cb (void *data,
                                        struct t_hashtable *hashtable,
                                        const void *key,
                                        const void *value)
{
    struct t_relay_client *ptr_client;
    int rc;
    long unsigned int buffer;
    struct t_hdata *ptr_hdata;
    struct t_relay_weechat_msg *msg;

    /* make C compiler happy */
    (void) hashtable;
    (void) value;

    ptr_client = (struct t_relay_client *)data;

    rc = sscanf (key, "%lx", &buffer);
    if ((rc != EOF) && (rc != 0))
    {
        ptr_hdata = weechat_hdata_get ("buffer");
        if (ptr_hdata)
        {
            if (weechat_hdata_check_pointer (ptr_hdata,
                                             weechat_hdata_get_list (ptr_hdata, "gui_buffers"),
                                             (void *)buffer))
            {
                msg = relay_weechat_msg_new ("_nicklist");
                if (msg)
                {
                    relay_weechat_msg_add_nicklist (msg, (struct t_gui_buffer *)buffer);
                    relay_weechat_msg_send (ptr_client, msg, 1);
                    relay_weechat_msg_free (msg);
                }
            }
        }
    }
}

/*
 * relay_weechat_protocol_timer_nicklist_cb: callback for nicklist timer
 */

int
relay_weechat_protocol_timer_nicklist_cb (void *data, int remaining_calls)
{
    struct t_relay_client *ptr_client;

    /* make C compiler happy */
    (void) remaining_calls;

    ptr_client = (struct t_relay_client *)data;
    if (!ptr_client || !relay_client_valid (ptr_client))
        return WEECHAT_RC_OK;

    weechat_hashtable_map (RELAY_WEECHAT_DATA(ptr_client, buffers_nicklist),
                           &relay_weechat_protocol_nicklist_map_cb,
                           ptr_client);

    weechat_hashtable_remove_all (RELAY_WEECHAT_DATA(ptr_client, buffers_nicklist));

    RELAY_WEECHAT_DATA(ptr_client, hook_timer_nicklist) = NULL;

    return WEECHAT_RC_OK;
}

/*
 * relay_weechat_protocol_signal_nicklist_cb: callback for "nicklist_*" signals
 */

int
relay_weechat_protocol_signal_nicklist_cb (void *data, const char *signal,
                                           const char *type_data,
                                           void *signal_data)
{
    struct t_relay_client *ptr_client;
    char *pos, *str_buffer;

    /* make C compiler happy */
    (void) signal;
    (void) type_data;

    ptr_client = (struct t_relay_client *)data;
    if (!ptr_client || !relay_client_valid (ptr_client))
        return WEECHAT_RC_OK;

    pos = strchr ((char *)signal_data, ',');
    if (!pos)
        return WEECHAT_RC_OK;

    str_buffer = weechat_strndup (signal_data, pos - (char *)signal_data);
    if (!str_buffer)
        return WEECHAT_RC_OK;
    weechat_hashtable_set (RELAY_WEECHAT_DATA(ptr_client, buffers_nicklist),
                           str_buffer, "1");
    free (str_buffer);

    if (RELAY_WEECHAT_DATA(ptr_client, hook_timer_nicklist))
    {
        weechat_unhook (RELAY_WEECHAT_DATA(ptr_client, hook_timer_nicklist));
        RELAY_WEECHAT_DATA(ptr_client, hook_timer_nicklist) = NULL;
    }
    relay_weechat_hook_timer_nicklist (ptr_client);

    return WEECHAT_RC_OK;
}

/*
 * relay_weechat_protocol_signal_upgrade_cb: callback for "upgrade*" signals
 */

int
relay_weechat_protocol_signal_upgrade_cb (void *data, const char *signal,
                                          const char *type_data,
                                          void *signal_data)
{
    struct t_relay_client *ptr_client;
    struct t_relay_weechat_msg *msg;
    char str_signal[128];

    /* make C compiler happy */
    (void) type_data;
    (void) signal_data;

    ptr_client = (struct t_relay_client *)data;
    if (!ptr_client || !relay_client_valid (ptr_client))
        return WEECHAT_RC_OK;

    snprintf (str_signal, sizeof (str_signal), "_%s", signal);

    if ((strcmp (signal, "upgrade") == 0)
        || (strcmp (signal, "upgrade_ended") == 0))
    {
        msg = relay_weechat_msg_new (str_signal);
        if (msg)
        {
            relay_weechat_msg_send (ptr_client, msg, 0);
            relay_weechat_msg_free (msg);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * relay_weechat_protocol_cb_sync: 'sync' command from client
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(sync)
{
    char **buffers, **flags, *full_name;
    int num_buffers, num_flags, i, add_flags, *ptr_old_flags, new_flags;
    struct t_gui_buffer *ptr_buffer;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(0);

    buffers = weechat_string_split ((argc > 0) ? argv[1] : "*", ",", 0, 0,
                                    &num_buffers);
    if (buffers)
    {
        add_flags = RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER |
            RELAY_WEECHAT_PROTOCOL_SYNC_NICKLIST;
        if (argc > 1)
        {
            flags = weechat_string_split (argv[2], ",", 0, 0, &num_flags);
            if (flags)
            {
                add_flags = 0;
                for (i = 0; i < num_flags; i++)
                {
                    if (strcmp (flags[i], "buffer") == 0)
                        add_flags |= RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER;
                    else if (strcmp (flags[i], "nicklist") == 0)
                        add_flags |= RELAY_WEECHAT_PROTOCOL_SYNC_NICKLIST;
                }
                if (add_flags == 0)
                {
                    add_flags = RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER |
                        RELAY_WEECHAT_PROTOCOL_SYNC_NICKLIST;
                }
                weechat_string_free_split (flags);
            }
        }
        for (i = 0; i < num_buffers; i++)
        {
            full_name = NULL;
            if (strcmp (buffers[i], "*") == 0)
            {
                full_name = strdup ("*");
            }
            else
            {
                ptr_buffer = relay_weechat_protocol_get_buffer (buffers[i]);
                if (ptr_buffer)
                    full_name = strdup (weechat_buffer_get_string (ptr_buffer,
                                                                   "full_name"));
            }
            if (full_name)
            {
                ptr_old_flags = weechat_hashtable_get (RELAY_WEECHAT_DATA(client, buffers_sync),
                                                       full_name);
                new_flags = ((ptr_old_flags) ? *ptr_old_flags : 0);
                new_flags |= add_flags;
                weechat_hashtable_set (RELAY_WEECHAT_DATA(client, buffers_sync),
                                       full_name,
                                       &new_flags);
                free (full_name);
            }
        }
        weechat_string_free_split (buffers);
    }

    if (!RELAY_WEECHAT_DATA(client, hook_signal_buffer))
        relay_weechat_hook_signals (client);

    return WEECHAT_RC_OK;
}

/*
 * relay_weechat_protocol_cb_desync: 'desync' command from client
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(desync)
{
    char **buffers, **flags, *full_name;
    int num_buffers, num_flags, i, sub_flags, *ptr_old_flags, new_flags;
    struct t_gui_buffer *ptr_buffer;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(0);

    buffers = weechat_string_split ((argc > 0) ? argv[1] : "*", ",", 0, 0,
                                    &num_buffers);
    if (buffers)
    {
        sub_flags = RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER |
            RELAY_WEECHAT_PROTOCOL_SYNC_NICKLIST;
        if (argc > 1)
        {
            flags = weechat_string_split (argv[2], ",", 0, 0, &num_flags);
            if (flags)
            {
                sub_flags = 0;
                for (i = 0; i < num_flags; i++)
                {
                    if (strcmp (flags[i], "buffer") == 0)
                        sub_flags |= RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER;
                    else if (strcmp (flags[i], "nicklist") == 0)
                        sub_flags |= RELAY_WEECHAT_PROTOCOL_SYNC_NICKLIST;
                }
                if (sub_flags == 0)
                {
                    sub_flags = RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER |
                        RELAY_WEECHAT_PROTOCOL_SYNC_NICKLIST;
                }
                weechat_string_free_split (flags);
            }
        }
        for (i = 0; i < num_buffers; i++)
        {
            full_name = NULL;
            if (strcmp (buffers[i], "*") == 0)
            {
                full_name = strdup ("*");
            }
            else
            {
                ptr_buffer = relay_weechat_protocol_get_buffer (buffers[i]);
                if (ptr_buffer)
                    full_name = strdup (weechat_buffer_get_string (ptr_buffer,
                                                                   "full_name"));
            }
            if (full_name)
            {
                ptr_old_flags = weechat_hashtable_get (RELAY_WEECHAT_DATA(client, buffers_sync),
                                                       full_name);
                new_flags = ((ptr_old_flags) ? *ptr_old_flags : 0);
                new_flags &= ~sub_flags;
                if (new_flags)
                {
                    weechat_hashtable_set (RELAY_WEECHAT_DATA(client, buffers_sync),
                                           full_name,
                                           &new_flags);
                }
                else
                {
                    weechat_hashtable_remove (RELAY_WEECHAT_DATA(client, buffers_sync),
                                              full_name);
                }
                free (full_name);
            }
        }
        weechat_string_free_split (buffers);
    }

    if (weechat_hashtable_get_integer (RELAY_WEECHAT_DATA(client, buffers_sync),
                                       "items_count") == 0)
    {
        relay_weechat_unhook_signals (client);
    }

    return WEECHAT_RC_OK;
}

/*
 * relay_weechat_protocol_cb_test: 'test' command from client
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(test)
{
    struct t_relay_weechat_msg *msg;

    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(0);

    msg = relay_weechat_msg_new (id);
    if (msg)
    {
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_CHAR);
        relay_weechat_msg_add_char (msg, 'A');
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_INT);
        relay_weechat_msg_add_int (msg, 123456);
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_LONG);
        relay_weechat_msg_add_long (msg, 1234567890L);
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_STRING);
        relay_weechat_msg_add_string (msg, "a string");
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_STRING);
        relay_weechat_msg_add_string (msg, "");
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_STRING);
        relay_weechat_msg_add_string (msg, NULL);
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_BUFFER);
        relay_weechat_msg_add_buffer (msg, "buffer", 6);
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_BUFFER);
        relay_weechat_msg_add_buffer (msg, NULL, 0);
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_POINTER);
        relay_weechat_msg_add_pointer (msg, &msg);
        relay_weechat_msg_add_type (msg, RELAY_WEECHAT_MSG_OBJ_TIME);
        relay_weechat_msg_add_time (msg, 1321993456);
        relay_weechat_msg_send (client, msg, 1);
        relay_weechat_msg_free (msg);
    }

    return WEECHAT_RC_OK;
}

/*
 * relay_weechat_protocol_cb_quit: 'quit' command from client
 */

RELAY_WEECHAT_PROTOCOL_CALLBACK(quit)
{
    RELAY_WEECHAT_PROTOCOL_MIN_ARGS(0);

    relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);

    return WEECHAT_RC_OK;
}

/*
 * relay_weechat_protocol_recv: read a command from client
 */

void
relay_weechat_protocol_recv (struct t_relay_client *client, char *data)
{
    char *pos, *id, *command, **argv, **argv_eol;
    int i, argc, return_code;
    struct t_relay_weechat_protocol_cb protocol_cb[] =
        { { "init", &relay_weechat_protocol_cb_init },
          { "hdata", &relay_weechat_protocol_cb_hdata },
          { "info", &relay_weechat_protocol_cb_info },
          { "infolist", &relay_weechat_protocol_cb_infolist },
          { "nicklist", &relay_weechat_protocol_cb_nicklist },
          { "input", &relay_weechat_protocol_cb_input },
          { "sync", &relay_weechat_protocol_cb_sync },
          { "desync", &relay_weechat_protocol_cb_desync },
          { "test", &relay_weechat_protocol_cb_test },
          { "quit", &relay_weechat_protocol_cb_quit },
          { NULL, NULL }
        };

    if (!data || !data[0] || RELAY_CLIENT_HAS_ENDED(client))
        return;

    /* remove \r at the end of message */
    pos = strchr (data, '\r');
    if (pos)
        pos[0] = '\0';

    /* display debug message */
    if (weechat_relay_plugin->debug >= 2)
    {
        weechat_printf (NULL, "%s: recv from client %d: \"%s\"",
                        RELAY_PLUGIN_NAME, client->id, data);
    }

    /* display message in raw buffer */
    relay_raw_print (client, RELAY_RAW_FLAG_RECV, "cmd: %s", data);

    /* extract id */
    id = NULL;
    if (data[0] == '(')
    {
        pos = strchr (data, ')');
        if (pos)
        {
            id = weechat_strndup (data + 1, pos - data - 1);
            data = pos + 1;
            while (data[0] == ' ')
            {
                data++;
            }
        }
    }

    /* search end of data */
    pos = strchr (data, ' ');
    if (pos)
        command = weechat_strndup (data, pos - data);
    else
        command = strdup (data);

    if (!command)
    {
        if (id)
            free (id);
        return;
    }

    argc = 0;
    argv = NULL;
    argv_eol = NULL;

    if (pos)
    {
        while (pos[0] == ' ')
        {
            pos++;
        }
        argv = weechat_string_split (pos, " ", 0, 0, &argc);
        argv_eol = weechat_string_split (pos, " ", 1, 0, NULL);
    }

    for (i = 0; protocol_cb[i].name; i++)
    {
        if (strcmp (protocol_cb[i].name, command) == 0)
        {
            if ((strcmp (protocol_cb[i].name, "init") != 0)
                && (!RELAY_WEECHAT_DATA(client, password_ok)))
            {
                /*
                 * command is not "init" and password is not set?
                 * then close connection!
                 */
                relay_client_set_status (client,
                                         RELAY_STATUS_DISCONNECTED);
            }
            else
            {
                return_code = (int) (protocol_cb[i].cmd_function) (client,
                                                                   id,
                                                                   protocol_cb[i].name,
                                                                   argc,
                                                                   argv,
                                                                   argv_eol);
                if ((weechat_relay_plugin->debug >= 1)
                    && (return_code == WEECHAT_RC_ERROR))
                {
                    weechat_printf (NULL,
                                    _("%s%s: failed to execute command \"%s\" "
                                      "for client %d"),
                                    weechat_prefix ("error"),
                                    RELAY_PLUGIN_NAME, command, client->id);
                }
            }
            break;
        }
    }

    if (id)
        free (id);
    if (argv)
        weechat_string_free_split (argv);
    if (argv_eol)
        weechat_string_free_split (argv_eol);
}
