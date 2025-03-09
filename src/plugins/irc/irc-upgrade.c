/*
 * irc-upgrade.c - save/restore IRC plugin data when upgrading WeeChat
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-upgrade.h"
#include "irc-buffer.h"
#include "irc-channel.h"
#include "irc-config.h"
#include "irc-input.h"
#include "irc-list.h"
#include "irc-modelist.h"
#include "irc-nick.h"
#include "irc-notify.h"
#include "irc-raw.h"
#include "irc-redirect.h"
#include "irc-server.h"


struct t_irc_server *irc_upgrade_current_server = NULL;
struct t_irc_channel *irc_upgrade_current_channel = NULL;
struct t_irc_modelist *irc_upgrade_current_modelist = NULL;
int irc_upgrading = 0;


/*
 * Saves servers/channels/nicks info to irc upgrade file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_upgrade_save_all_data (struct t_upgrade_file *upgrade_file,
                           int force_disconnected_state)
{
    struct t_infolist *infolist;
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    struct t_irc_redirect *ptr_redirect;
    struct t_irc_redirect_pattern *ptr_redirect_pattern;
    struct t_irc_notify *ptr_notify;
    struct t_irc_modelist *ptr_modelist;
    struct t_irc_modelist_item *ptr_item;
    struct t_irc_raw_message *ptr_raw_message;
    int rc;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        /* save server */
        infolist = weechat_infolist_new ();
        if (!infolist)
            return 0;
        if (!irc_server_add_to_infolist (infolist, ptr_server,
                                         force_disconnected_state))
        {
            weechat_infolist_free (infolist);
            return 0;
        }
        rc = weechat_upgrade_write_object (upgrade_file,
                                           IRC_UPGRADE_TYPE_SERVER,
                                           infolist);
        weechat_infolist_free (infolist);
        if (!rc)
            return 0;

        /* save server channels and nicks */
        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            /* save channel */
            infolist = weechat_infolist_new ();
            if (!infolist)
                return 0;
            if (!irc_channel_add_to_infolist (infolist, ptr_channel))
            {
                weechat_infolist_free (infolist);
                return 0;
            }
            rc = weechat_upgrade_write_object (upgrade_file,
                                               IRC_UPGRADE_TYPE_CHANNEL,
                                               infolist);
            weechat_infolist_free (infolist);
            if (!rc)
                return 0;

            if (!force_disconnected_state)
            {
                for (ptr_nick = ptr_channel->nicks; ptr_nick;
                     ptr_nick = ptr_nick->next_nick)
                {
                    /* save nick */
                    infolist = weechat_infolist_new ();
                    if (!infolist)
                        return 0;
                    if (!irc_nick_add_to_infolist (infolist, ptr_nick))
                    {
                        weechat_infolist_free (infolist);
                        return 0;
                    }
                    rc = weechat_upgrade_write_object (upgrade_file,
                                                       IRC_UPGRADE_TYPE_NICK,
                                                       infolist);
                    weechat_infolist_free (infolist);
                    if (!rc)
                        return 0;
                }

                for (ptr_modelist = ptr_channel->modelists; ptr_modelist;
                     ptr_modelist = ptr_modelist->next_modelist)
                {
                    /* save modelist */
                    infolist = weechat_infolist_new ();
                    if (!infolist)
                        return 0;
                    if (!irc_modelist_add_to_infolist (infolist, ptr_modelist))
                    {
                        weechat_infolist_free (infolist);
                        return 0;
                    }
                    rc = weechat_upgrade_write_object (upgrade_file,
                                                       IRC_UPGRADE_TYPE_MODELIST,
                                                       infolist);
                    weechat_infolist_free (infolist);
                    if (!rc)
                        return 0;

                    for (ptr_item = ptr_modelist->items; ptr_item;
                         ptr_item = ptr_item->next_item)
                    {
                        /* save modelist item */
                        infolist = weechat_infolist_new ();
                        if (!infolist)
                            return 0;
                        if (!irc_modelist_item_add_to_infolist (infolist, ptr_item))
                        {
                            weechat_infolist_free (infolist);
                            return 0;
                        }
                        rc = weechat_upgrade_write_object (upgrade_file,
                                                           IRC_UPGRADE_TYPE_MODELIST_ITEM,
                                                           infolist);
                        weechat_infolist_free (infolist);
                        if (!rc)
                            return 0;
                    }
                }
            }
        }

        /* save server redirects */
        for (ptr_redirect = ptr_server->redirects; ptr_redirect;
             ptr_redirect = ptr_redirect->next_redirect)
        {
            infolist = weechat_infolist_new ();
            if (!infolist)
                return 0;
            if (!irc_redirect_add_to_infolist (infolist, ptr_redirect))
            {
                weechat_infolist_free (infolist);
                return 0;
            }
            rc = weechat_upgrade_write_object (upgrade_file,
                                               IRC_UPGRADE_TYPE_REDIRECT,
                                               infolist);
            weechat_infolist_free (infolist);
            if (!rc)
                return 0;
        }

        /* save server notify list */
        for (ptr_notify = ptr_server->notify_list; ptr_notify;
             ptr_notify = ptr_notify->next_notify)
        {
            infolist = weechat_infolist_new ();
            if (!infolist)
                return 0;
            if (!irc_notify_add_to_infolist (infolist, ptr_notify))
            {
                weechat_infolist_free (infolist);
                return 0;
            }
            rc = weechat_upgrade_write_object (upgrade_file,
                                               IRC_UPGRADE_TYPE_NOTIFY,
                                               infolist);
            weechat_infolist_free (infolist);
            if (!rc)
                return 0;
        }
    }

    /* save raw messages */
    for (ptr_raw_message = irc_raw_messages; ptr_raw_message;
         ptr_raw_message = ptr_raw_message->next_message)
    {
        infolist = weechat_infolist_new ();
        if (!infolist)
            return 0;
        if (!irc_raw_add_to_infolist (infolist, ptr_raw_message))
        {
            weechat_infolist_free (infolist);
            return 0;
        }
        rc = weechat_upgrade_write_object (upgrade_file,
                                           IRC_UPGRADE_TYPE_RAW_MESSAGE,
                                           infolist);
        weechat_infolist_free (infolist);
        if (!rc)
            return 0;
    }

    /* save redirect patterns */
    for (ptr_redirect_pattern = irc_redirect_patterns; ptr_redirect_pattern;
         ptr_redirect_pattern = ptr_redirect_pattern->next_redirect)
    {
        /* save only temporary patterns (created by other plugins/scripts) */
        if (ptr_redirect_pattern->temp_pattern)
        {
            infolist = weechat_infolist_new ();
            if (!infolist)
                return 0;
            if (!irc_redirect_pattern_add_to_infolist (infolist, ptr_redirect_pattern))
            {
                weechat_infolist_free (infolist);
                return 0;
            }
            rc = weechat_upgrade_write_object (upgrade_file,
                                               IRC_UPGRADE_TYPE_REDIRECT_PATTERN,
                                               infolist);
            weechat_infolist_free (infolist);
            if (!rc)
                return 0;
        }
    }

    return 1;
}

/*
 * Saves irc upgrade file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_upgrade_save (int force_disconnected_state)
{
    int rc;
    struct t_upgrade_file *upgrade_file;

    upgrade_file = weechat_upgrade_new (IRC_UPGRADE_FILENAME,
                                        NULL, NULL, NULL);
    if (!upgrade_file)
        return 0;

    rc = irc_upgrade_save_all_data (upgrade_file, force_disconnected_state);

    weechat_upgrade_close (upgrade_file);

    return rc;
}

/*
 * Restores buffers callbacks (input and close) for buffers created by irc
 * plugin.
 */

void
irc_upgrade_set_buffer_callbacks (void)
{
    struct t_infolist *infolist;
    struct t_gui_buffer *ptr_buffer;
    struct t_irc_server *ptr_server;
    const char *type;

    infolist = weechat_infolist_get ("buffer", NULL, NULL);
    if (infolist)
    {
        while (weechat_infolist_next (infolist))
        {
            if (weechat_infolist_pointer (infolist, "plugin") == weechat_irc_plugin)
            {
                ptr_buffer = weechat_infolist_pointer (infolist, "pointer");
                weechat_buffer_set_pointer (ptr_buffer, "close_callback", &irc_buffer_close_cb);
                weechat_buffer_set_pointer (ptr_buffer, "input_callback", &irc_input_data_cb);
                type = weechat_buffer_get_string (ptr_buffer, "localvar_type");
                if (type && (strcmp (type, "channel") == 0))
                {
                    ptr_server = irc_server_search (
                        weechat_buffer_get_string (ptr_buffer,
                                                   "localvar_server"));
                    weechat_buffer_set_pointer (ptr_buffer, "nickcmp_callback",
                                                &irc_buffer_nickcmp_cb);
                    if (ptr_server)
                    {
                        weechat_buffer_set_pointer (ptr_buffer,
                                                    "nickcmp_callback_pointer",
                                                    ptr_server);
                    }
                }
                if (type && (strcmp (type, "list") == 0))
                {
                    ptr_server = irc_server_search (
                        weechat_buffer_get_string (ptr_buffer,
                                                   "localvar_server"));
                    if (ptr_server)
                        ptr_server->list->buffer = ptr_buffer;
                    irc_list_buffer_refresh (ptr_server, 1);
                }
                if (strcmp (weechat_infolist_string (infolist, "name"),
                            IRC_RAW_BUFFER_NAME) == 0)
                {
                    irc_raw_buffer = ptr_buffer;
                }
            }
        }
        weechat_infolist_free (infolist);
    }
}

/*
 * Reads callback for irc upgrade.
 */

int
irc_upgrade_read_cb (const void *pointer, void *data,
                     struct t_upgrade_file *upgrade_file,
                     int object_id,
                     struct t_infolist *infolist)
{
    int flags, sock, size, i, index, nicks_count, num_items, utf8mapping;
    long number;
    time_t join_time;
    char *buf, option_name[64], **nicks, *nick_join, *pos, *error;
    char **items;
    const char *buffer_name, *str, *nick;
    struct t_irc_server *ptr_server;
    struct t_irc_nick *ptr_nick;
    struct t_irc_redirect *ptr_redirect;
    struct t_irc_notify *ptr_notify;
    struct t_irc_modelist_item *ptr_item;
    struct t_gui_buffer *ptr_buffer;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) upgrade_file;

    weechat_infolist_reset_item_cursor (infolist);
    while (weechat_infolist_next (infolist))
    {
        switch (object_id)
        {
            case IRC_UPGRADE_TYPE_SERVER:
                irc_upgrade_current_server = irc_server_search (weechat_infolist_string (infolist, "name"));
                if (irc_upgrade_current_server)
                {
                    irc_upgrade_current_server->temp_server =
                        weechat_infolist_integer (infolist, "temp_server");
                    irc_upgrade_current_server->fake_server =
                        weechat_infolist_integer (infolist, "fake_server");
                    irc_upgrade_current_server->buffer = NULL;
                    buffer_name = weechat_infolist_string (infolist, "buffer_name");
                    if (buffer_name && buffer_name[0])
                    {
                        ptr_buffer = weechat_buffer_search (IRC_PLUGIN_NAME,
                                                            buffer_name);
                        if (ptr_buffer)
                            irc_upgrade_current_server->buffer = ptr_buffer;
                    }
                    irc_upgrade_current_server->index_current_address =
                        weechat_infolist_integer (infolist, "index_current_address");
                    str = weechat_infolist_string (infolist, "current_address");
                    if (str)
                    {
                        irc_upgrade_current_server->current_address = strdup (str);
                        irc_upgrade_current_server->current_port = weechat_infolist_integer (infolist, "current_port");
                    }
                    else
                    {
                        if (irc_upgrade_current_server->index_current_address < irc_upgrade_current_server->addresses_count)
                        {
                            irc_upgrade_current_server->current_address =
                                strdup (irc_upgrade_current_server->addresses_array[irc_upgrade_current_server->index_current_address]);
                            irc_upgrade_current_server->current_port =
                                irc_upgrade_current_server->ports_array[irc_upgrade_current_server->index_current_address];
                        }
                    }
                    str = weechat_infolist_string (infolist, "current_ip");
                    if (str)
                        irc_upgrade_current_server->current_ip = strdup (str);
                    sock = weechat_infolist_integer (infolist, "sock");
                    if (sock >= 0)
                    {
                        irc_upgrade_current_server->sock = sock;
                        irc_upgrade_current_server->hook_fd = weechat_hook_fd (
                            irc_upgrade_current_server->sock,
                            1, 0, 0,
                            &irc_server_recv_cb,
                            irc_upgrade_current_server,
                            NULL);
                    }
                    /*
                     * "authentication_method" and "sasl_mechanism_used" are
                     * new in WeeChat 4.0.0
                     */
                    if (weechat_infolist_search_var (infolist, "authentication_method"))
                    {
                        irc_upgrade_current_server->authentication_method = weechat_infolist_integer (infolist, "authentication_method");
                        irc_upgrade_current_server->sasl_mechanism_used = weechat_infolist_integer (infolist, "sasl_mechanism_used");
                    }
                    else
                    {
                        irc_upgrade_current_server->authentication_method = IRC_SERVER_AUTH_METHOD_NONE;
                        irc_upgrade_current_server->sasl_mechanism_used = -1;
                    }
                    irc_upgrade_current_server->is_connected = weechat_infolist_integer (infolist, "is_connected");
                    /* "tls_connected" replaces "ssl_connected" in WeeChat 4.0.0 */
                    if (weechat_infolist_search_var (infolist, "tls_connected"))
                        irc_upgrade_current_server->tls_connected = weechat_infolist_integer (infolist, "tls_connected");
                    else
                        irc_upgrade_current_server->tls_connected = weechat_infolist_integer (infolist, "ssl_connected");
                    irc_upgrade_current_server->disconnected = weechat_infolist_integer (infolist, "disconnected");
                    str = weechat_infolist_string (infolist, "unterminated_message");
                    if (str)
                        irc_upgrade_current_server->unterminated_message = strdup (str);
                    str = weechat_infolist_string (infolist, "nick");
                    if (str)
                        irc_server_set_nick (irc_upgrade_current_server, str);
                    str = weechat_infolist_string (infolist, "nick_modes");
                    if (str)
                        irc_upgrade_current_server->nick_modes = strdup (str);
                    str = weechat_infolist_string (infolist, "host");
                    if (str)
                        irc_upgrade_current_server->host = strdup (str);
                    /*
                     * "cap_ls" and "cap_list" replace "cap_away_notify",
                     * "cap_account_notify" and "cap_extended_join"
                     * in WeeChat 2.2
                     */
                    if (weechat_infolist_integer (infolist, "cap_away_notify"))
                    {
                        weechat_hashtable_set (irc_upgrade_current_server->cap_ls, "away-notify", NULL);
                        weechat_hashtable_set (irc_upgrade_current_server->cap_list, "away-notify", NULL);
                    }
                    if (weechat_infolist_integer (infolist, "cap_account_notify"))
                    {
                        weechat_hashtable_set (irc_upgrade_current_server->cap_ls, "account-notify", NULL);
                        weechat_hashtable_set (irc_upgrade_current_server->cap_list, "account-notify", NULL);
                    }
                    if (weechat_infolist_integer (infolist, "cap_extended_join"))
                    {
                        weechat_hashtable_set (irc_upgrade_current_server->cap_ls, "extended-join", NULL);
                        weechat_hashtable_set (irc_upgrade_current_server->cap_list, "extended-join", NULL);
                    }
                    weechat_hashtable_add_from_infolist (
                        irc_upgrade_current_server->cap_ls, infolist, "cap_ls");
                    weechat_hashtable_add_from_infolist (
                        irc_upgrade_current_server->cap_list, infolist, "cap_list");

                    str = weechat_infolist_string (infolist, "isupport");
                    if (str)
                        irc_upgrade_current_server->isupport = strdup (str);
                    /*
                     * "prefix" is not anymore in this infolist (since
                     * WeeChat 0.3.4), but we read it to keep compatibility
                     * with old WeeChat versions, on /upgrade)
                     */
                    str = weechat_infolist_string (infolist, "prefix");
                    if (str)
                        irc_server_set_prefix_modes_chars (irc_upgrade_current_server, str);
                    /* "prefix_modes" is new in WeeChat 0.3.4 */
                    str = weechat_infolist_string (infolist, "prefix_modes");
                    if (str)
                    {
                        free (irc_upgrade_current_server->prefix_modes);
                        irc_upgrade_current_server->prefix_modes = strdup (str);
                    }
                    /* "prefix_chars" is new in WeeChat 0.3.4 */
                    str = weechat_infolist_string (infolist, "prefix_chars");
                    if (str)
                    {
                        free (irc_upgrade_current_server->prefix_chars);
                        irc_upgrade_current_server->prefix_chars = strdup (str);
                    }
                    /* "msg_max_length" is new in WeeChat 4.0.0 */
                    if (weechat_infolist_search_var (infolist, "msg_max_length"))
                    {
                        irc_upgrade_current_server->msg_max_length = weechat_infolist_integer (infolist, "msg_max_length");
                    }
                    else
                    {
                        /* WeeChat <= 3.8 */
                        str = irc_server_get_isupport_value (irc_upgrade_current_server,
                                                             "LINELEN");
                        if (str)
                        {
                            error = NULL;
                            number = strtol (str, &error, 10);
                            if (error && !error[0])
                                irc_upgrade_current_server->msg_max_length = (int)number;
                        }
                    }
                    irc_upgrade_current_server->nick_max_length = weechat_infolist_integer (infolist, "nick_max_length");
                    /* "user_max_length" is new in WeeChat 2.6 */
                    if (weechat_infolist_search_var (infolist, "user_max_length"))
                    {
                        irc_upgrade_current_server->user_max_length = weechat_infolist_integer (infolist, "user_max_length");
                    }
                    else
                    {
                        /* WeeChat <= 2.5 */
                        str = irc_server_get_isupport_value (irc_upgrade_current_server,
                                                             "USERLEN");
                        if (str)
                        {
                            error = NULL;
                            number = strtol (str, &error, 10);
                            if (error && !error[0])
                                irc_upgrade_current_server->user_max_length = (int)number;
                        }
                    }
                    /* "host_max_length" is new in WeeChat 2.6 */
                    if (weechat_infolist_search_var (infolist, "host_max_length"))
                    {
                        irc_upgrade_current_server->host_max_length = weechat_infolist_integer (infolist, "host_max_length");
                    }
                    else
                    {
                        /* WeeChat <= 2.5 */
                        str = irc_server_get_isupport_value (irc_upgrade_current_server,
                                                             "HOSTLEN");
                        if (str)
                        {
                            error = NULL;
                            number = strtol (str, &error, 10);
                            if (error && !error[0])
                                irc_upgrade_current_server->host_max_length = (int)number;
                        }
                    }
                    irc_upgrade_current_server->casemapping = weechat_infolist_integer (infolist, "casemapping");
                    /* "utf8mapping" is new in WeeChat 2.9 */
                    if (weechat_infolist_search_var (infolist, "utf8mapping"))
                    {
                        irc_upgrade_current_server->utf8mapping = weechat_infolist_integer (infolist, "utf8mapping");
                    }
                    else
                    {
                        /* WeeChat <= 2.8 */
                        str = irc_server_get_isupport_value (irc_upgrade_current_server,
                                                             "UTF8MAPPING");
                        if (str)
                        {
                            utf8mapping = irc_server_search_utf8mapping (str);
                            if (utf8mapping >= 0)
                                irc_upgrade_current_server->utf8mapping = utf8mapping;
                        }
                    }
                    /* "utf8only" is new in WeeChat 4.0.0 */
                    if (weechat_infolist_search_var (infolist, "utf8only"))
                    {
                        irc_upgrade_current_server->utf8only = weechat_infolist_integer (infolist, "utf8only");
                    }
                    else
                    {
                        /* WeeChat <= 3.8 */
                        irc_upgrade_current_server->utf8only = (
                            irc_server_get_isupport_value (
                                irc_upgrade_current_server,
                                "UTF8ONLY")) ?
                            1 : 0;
                    }
                    str = weechat_infolist_string (infolist, "chantypes");
                    if (str)
                        irc_upgrade_current_server->chantypes = strdup (str);
                    str = weechat_infolist_string (infolist, "chanmodes");
                    if (str)
                        irc_upgrade_current_server->chanmodes = strdup (str);
                    else
                    {
                        str = irc_server_get_isupport_value (irc_upgrade_current_server,
                                                             "CHANMODES");
                        if (str)
                            irc_upgrade_current_server->chanmodes = strdup (str);
                    }
                    /* "monitor" is new in WeeChat 0.4.3 */
                    if (weechat_infolist_search_var (infolist, "monitor"))
                    {
                        irc_upgrade_current_server->monitor = weechat_infolist_integer (infolist, "monitor");
                    }
                    else
                    {
                        /* WeeChat <= 0.4.2 */
                        str = irc_server_get_isupport_value (irc_upgrade_current_server,
                                                             "MONITOR");
                        if (str)
                        {
                            error = NULL;
                            number = strtol (str, &error, 10);
                            if (error && !error[0])
                                irc_upgrade_current_server->monitor = (int)number;
                        }
                    }
                    /* "clienttagdeny" is new in WeeChat 3.3 */
                    if (weechat_infolist_search_var (infolist, "clienttagdeny"))
                    {
                        irc_server_set_clienttagdeny (irc_upgrade_current_server,
                                                      weechat_infolist_string (infolist, "clienttagdeny"));
                    }
                    else
                    {
                        /* WeeChat <= 3.2 */
                        str = irc_server_get_isupport_value (irc_upgrade_current_server,
                                                             "CLIENTTAGDENY");
                        if (str)
                        {
                            irc_server_set_clienttagdeny (irc_upgrade_current_server,
                                                          str);
                        }
                    }
                    irc_upgrade_current_server->reconnect_delay = weechat_infolist_integer (infolist, "reconnect_delay");
                    irc_upgrade_current_server->reconnect_start = weechat_infolist_time (infolist, "reconnect_start");
                    irc_upgrade_current_server->command_time = weechat_infolist_time (infolist, "command_time");
                    irc_upgrade_current_server->autojoin_time = weechat_infolist_time (infolist, "autojoin_time");
                    irc_upgrade_current_server->autojoin_done = weechat_infolist_integer (infolist, "autojoin_done");
                    irc_upgrade_current_server->disable_autojoin = weechat_infolist_integer (infolist, "disable_autojoin");
                    irc_upgrade_current_server->is_away = weechat_infolist_integer (infolist, "is_away");
                    str = weechat_infolist_string (infolist, "away_message");
                    if (str)
                        irc_upgrade_current_server->away_message = strdup (str);
                    irc_upgrade_current_server->away_time = weechat_infolist_time (infolist, "away_time");
                    irc_upgrade_current_server->lag = weechat_infolist_integer (infolist, "lag");
                    irc_upgrade_current_server->lag_displayed = weechat_infolist_integer (infolist, "lag_displayed");
                    buf = weechat_infolist_buffer (infolist, "lag_check_time", &size);
                    if (buf)
                        memcpy (&(irc_upgrade_current_server->lag_check_time), buf, size);
                    irc_upgrade_current_server->lag_next_check = weechat_infolist_time (infolist, "lag_next_check");
                    irc_upgrade_current_server->lag_last_refresh = weechat_infolist_time (infolist, "lag_last_refresh");
                    irc_upgrade_current_server->last_away_check = weechat_infolist_time (infolist, "last_away_check");
                    irc_upgrade_current_server->last_data_purge = weechat_infolist_time (infolist, "last_data_purge");
                }
                break;
            case IRC_UPGRADE_TYPE_CHANNEL:
                if (irc_upgrade_current_server)
                {
                    irc_upgrade_current_channel = irc_channel_new (irc_upgrade_current_server,
                                                                   weechat_infolist_integer (infolist, "type"),
                                                                   weechat_infolist_string (infolist, "name"),
                                                                   0, 0);
                    if (irc_upgrade_current_channel)
                    {
                        str = weechat_infolist_string (infolist, "topic");
                        if (str)
                            irc_channel_set_topic (irc_upgrade_current_channel, str);
                        str = weechat_infolist_string (infolist, "modes");
                        if (str)
                            irc_upgrade_current_channel->modes = strdup (str);
                        irc_upgrade_current_channel->limit = weechat_infolist_integer (infolist, "limit");
                        str = weechat_infolist_string (infolist, "key");
                        if (str)
                            irc_upgrade_current_channel->key = strdup (str);
                        str = weechat_infolist_string (infolist, "join_msg_received");
                        if (str)
                        {
                            items = weechat_string_split (
                                str,
                                ",",
                                NULL,
                                WEECHAT_STRING_SPLIT_STRIP_LEFT
                                | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                0,
                                &num_items);
                            if (items)
                            {
                                for (i = 0; i < num_items; i++)
                                {
                                    weechat_hashtable_set (irc_upgrade_current_channel->join_msg_received,
                                                           items[i], "1");
                                }
                                weechat_string_free_split (items);
                            }
                        }
                        irc_upgrade_current_channel->checking_whox = weechat_infolist_integer (infolist, "checking_whox");
                        str = weechat_infolist_string (infolist, "away_message");
                        if (str)
                            irc_upgrade_current_channel->away_message = strdup (str);
                        irc_upgrade_current_channel->has_quit_server = weechat_infolist_integer (infolist, "has_quit_server");
                        irc_upgrade_current_channel->cycle = weechat_infolist_integer (infolist, "cycle");
                        irc_upgrade_current_channel->part = weechat_infolist_integer (infolist, "part");
                        irc_upgrade_current_channel->nick_completion_reset = weechat_infolist_integer (infolist, "nick_completion_reset");
                        for (i = 0; i < 2; i++)
                        {
                            index = 0;
                            while (1)
                            {
                                snprintf (option_name, sizeof (option_name),
                                          "nick_speaking%d_%05d", i, index);
                                nick = weechat_infolist_string (infolist, option_name);
                                if (!nick)
                                    break;
                                irc_channel_nick_speaking_add (irc_upgrade_current_channel,
                                                               nick,
                                                               i);
                                index++;
                            }
                        }
                        index = 0;
                        while (1)
                        {
                            snprintf (option_name, sizeof (option_name),
                                      "nick_speaking_time_nick_%05d", index);
                            nick = weechat_infolist_string (infolist, option_name);
                            if (!nick)
                                break;
                            snprintf (option_name, sizeof (option_name),
                                      "nick_speaking_time_time_%05d", index);
                            irc_channel_nick_speaking_time_add (irc_upgrade_current_server,
                                                                irc_upgrade_current_channel,
                                                                nick,
                                                                weechat_infolist_time (infolist,
                                                                                       option_name));
                            index++;
                        }
                        str = weechat_infolist_string (infolist, "join_smart_filtered");
                        if (str)
                        {
                            nicks = weechat_string_split (
                                str,
                                ",",
                                NULL,
                                WEECHAT_STRING_SPLIT_STRIP_LEFT
                                | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                0,
                                &nicks_count);
                            if (nicks)
                            {
                                for (i = 0; i < nicks_count; i++)
                                {
                                    pos = strchr (nicks[i], ':');
                                    if (pos)
                                    {
                                        nick_join = weechat_strndup (nicks[i],
                                                                     pos - nicks[i]);
                                        if (nick_join)
                                        {
                                            error = NULL;
                                            number = strtol (pos + 1, &error, 10);
                                            if (error && !error[0])
                                            {
                                                join_time = (time_t)number;
                                                irc_channel_join_smart_filtered_add (irc_upgrade_current_channel,
                                                                                     nick_join,
                                                                                     join_time);
                                            }
                                            free (nick_join);
                                        }
                                    }
                                }
                                weechat_string_free_split (nicks);
                            }
                        }
                    }
                }
                break;
            case IRC_UPGRADE_TYPE_NICK:
                if (irc_upgrade_current_server && irc_upgrade_current_channel)
                {
                    ptr_nick = irc_nick_new_in_channel (
                        irc_upgrade_current_server,
                        irc_upgrade_current_channel,
                        weechat_infolist_string (infolist, "name"),
                        weechat_infolist_string (infolist, "host"),
                        weechat_infolist_string (infolist, "prefixes"),
                        weechat_infolist_integer (infolist, "away"),
                        weechat_infolist_string (infolist, "account"),
                        weechat_infolist_string (infolist, "realname"));
                    if (ptr_nick)
                    {
                        /*
                         * "flags" is not anymore in this infolist (since
                         * WeeChat 0.3.4), but we read it to keep compatibility
                         * with old WeeChat versions, on /upgrade)
                         * We try to restore prefixes with old flags, but
                         * this is approximation, it's not sure we will
                         * restore good prefixes here (a /names on channel
                         * will fix problem if prefixes are wrong).
                         * Flags were defined in irc-nick.h:
                         *   #define IRC_NICK_CHANOWNER  1
                         *   #define IRC_NICK_CHANADMIN  2
                         *   #define IRC_NICK_CHANADMIN2 4
                         *   #define IRC_NICK_OP         8
                         *   #define IRC_NICK_HALFOP     16
                         *   #define IRC_NICK_VOICE      32
                         *   #define IRC_NICK_AWAY       64
                         *   #define IRC_NICK_CHANUSER   128
                         */
                        flags = weechat_infolist_integer (infolist, "flags");
                        if (flags > 0)
                        {
                            /* channel owner */
                            if (flags & 1)
                            {
                                irc_nick_set_mode (irc_upgrade_current_server,
                                                   irc_upgrade_current_channel,
                                                   ptr_nick, 1, 'q');
                            }
                            /* channel admin */
                            if ((flags & 2) || (flags & 4))
                            {
                                irc_nick_set_mode (irc_upgrade_current_server,
                                                   irc_upgrade_current_channel,
                                                   ptr_nick, 1, 'a');
                            }
                            /* op */
                            if (flags & 8)
                            {
                                irc_nick_set_mode (irc_upgrade_current_server,
                                                   irc_upgrade_current_channel,
                                                   ptr_nick, 1, 'o');
                            }
                            /* half-op */
                            if (flags & 16)
                            {
                                irc_nick_set_mode (irc_upgrade_current_server,
                                                   irc_upgrade_current_channel,
                                                   ptr_nick, 1, 'h');
                            }
                            /* voice */
                            if (flags & 32)
                            {
                                irc_nick_set_mode (irc_upgrade_current_server,
                                                   irc_upgrade_current_channel,
                                                   ptr_nick, 1, 'v');
                            }
                            /* away */
                            if (flags & 64)
                            {
                                irc_nick_set_away (irc_upgrade_current_server,
                                                   irc_upgrade_current_channel,
                                                   ptr_nick, 1);
                            }
                            /* channel user */
                            if (flags & 128)
                            {
                                irc_nick_set_mode (irc_upgrade_current_server,
                                                   irc_upgrade_current_channel,
                                                   ptr_nick, 1, 'u');
                            }
                        }
                    }
                }
                break;
            case IRC_UPGRADE_TYPE_MODELIST:
                if (irc_upgrade_current_server && irc_upgrade_current_channel)
                {
                    /* modelists are already created by the channel */
                    irc_upgrade_current_modelist = irc_modelist_search (
                        irc_upgrade_current_channel,
                        weechat_infolist_string (infolist, "type")[0]);
                    if (irc_upgrade_current_modelist)
                    {
                        irc_upgrade_current_modelist->state = weechat_infolist_integer (infolist, "state");
                    }
                }
                break;
            case IRC_UPGRADE_TYPE_MODELIST_ITEM:
                if (irc_upgrade_current_server && irc_upgrade_current_channel && irc_upgrade_current_modelist)
                {
                    ptr_item = irc_modelist_item_new (
                        irc_upgrade_current_modelist,
                        weechat_infolist_string (infolist, "mask"),
                        weechat_infolist_string (infolist, "setter"),
                        weechat_infolist_time (infolist, "datetime"));
                    if (ptr_item)
                    {
                        ptr_item->number = weechat_infolist_integer (infolist, "number");
                    }
                }
                break;
            case IRC_UPGRADE_TYPE_REDIRECT:
                if (irc_upgrade_current_server)
                {
                    ptr_redirect = irc_redirect_new_with_commands (
                        irc_upgrade_current_server,
                        weechat_infolist_string (infolist, "pattern"),
                        weechat_infolist_string (infolist, "signal"),
                        weechat_infolist_integer (infolist, "count"),
                        weechat_infolist_string (infolist, "string"),
                        weechat_infolist_integer (infolist, "timeout"),
                        weechat_infolist_string (infolist, "cmd_start"),
                        weechat_infolist_string (infolist, "cmd_stop"),
                        weechat_infolist_string (infolist, "cmd_extra"),
                        weechat_infolist_string (infolist, "cmd_filter"));
                    if (ptr_redirect)
                    {
                        ptr_redirect->current_count = weechat_infolist_integer (infolist, "current_count");
                        str = weechat_infolist_string (infolist, "command");
                        if (str)
                            ptr_redirect->command = strdup (str);
                        ptr_redirect->assigned_to_command = weechat_infolist_integer (infolist, "assigned_to_command");
                        ptr_redirect->start_time = weechat_infolist_time (infolist, "start_time");
                        ptr_redirect->cmd_start_received = weechat_infolist_integer (infolist, "cmd_start_received");
                        ptr_redirect->cmd_stop_received = weechat_infolist_integer (infolist, "cmd_stop_received");
                        str = weechat_infolist_string (infolist, "output");
                        if (str)
                            ptr_redirect->output = strdup (str);
                        ptr_redirect->output_size = weechat_infolist_integer (infolist, "output_size");
                    }
                }
                break;
            case IRC_UPGRADE_TYPE_REDIRECT_PATTERN:
                irc_redirect_pattern_new (
                    weechat_infolist_string (infolist, "name"),
                    weechat_infolist_integer (infolist, "temp_pattern"),
                    weechat_infolist_integer (infolist, "timeout"),
                    weechat_infolist_string (infolist, "cmd_start"),
                    weechat_infolist_string (infolist, "cmd_stop"),
                    weechat_infolist_string (infolist, "cmd_extra"));
                break;
            case IRC_UPGRADE_TYPE_NOTIFY:
                if (irc_upgrade_current_server)
                {
                    ptr_notify = irc_notify_search (irc_upgrade_current_server,
                                                    weechat_infolist_string (infolist, "nick"));
                    if (ptr_notify)
                    {
                        ptr_notify->is_on_server = weechat_infolist_integer (infolist, "is_on_server");
                        str = weechat_infolist_string (infolist, "away_message");
                        if (str)
                            ptr_notify->away_message = strdup (str);
                    }
                }
                break;
            case IRC_UPGRADE_TYPE_RAW_MESSAGE:
                /* "server" and "flags" are new in WeeChat 2.7  */
                str = weechat_infolist_string (infolist, "server");
                if (str && str[0])
                {
                    ptr_server = irc_server_search (str);
                    if (ptr_server)
                    {
                        irc_raw_message_add_to_list (
                            weechat_infolist_time (infolist, "date"),
                            weechat_infolist_integer (infolist, "date_usec"),
                            ptr_server,
                            weechat_infolist_integer (infolist, "flags"),
                            weechat_infolist_string (infolist, "message"));
                    }
                }
                break;
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Set buffer properties on IRC buffers after upgrade:
 *   - "input_prompt" (introduced in WeeChat 4.3.0)
 *   - "modes" (introduced in WeeChat 4.3.0)
 */

void
irc_upgrade_set_buffer_properties (void)
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        /* set input prompt on server and all channels */
        if (ptr_server->buffer)
            irc_server_set_buffer_input_prompt (ptr_server);

        /* set modes on all channels */
        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if (ptr_channel->buffer)
                irc_channel_set_buffer_modes (ptr_server, ptr_channel);
        }
    }
}

/*
 * Loads irc upgrade file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_upgrade_load (void)
{
    int rc;
    struct t_upgrade_file *upgrade_file;
    const char *ptr_filter;

    irc_upgrade_set_buffer_callbacks ();

    upgrade_file = weechat_upgrade_new (IRC_UPGRADE_FILENAME,
                                        &irc_upgrade_read_cb, NULL, NULL);
    if (!upgrade_file)
        return 0;

    irc_upgrading = 1;
    rc = weechat_upgrade_read (upgrade_file);
    irc_upgrading = 0;

    weechat_upgrade_close (upgrade_file);

    if (irc_raw_buffer)
    {
        ptr_filter = weechat_buffer_get_string (irc_raw_buffer,
                                                "localvar_filter");
        irc_raw_filter_options (
            (ptr_filter && ptr_filter[0]) ? ptr_filter : "*");
    }

    irc_upgrade_set_buffer_properties ();

    return rc;
}
