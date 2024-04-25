/*
 * xfer.c - file transfer and direct chat plugin for WeeChat
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
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <gcrypt.h>
#include <arpa/inet.h>

#include "../weechat-plugin.h"
#include "xfer.h"
#include "xfer-buffer.h"
#include "xfer-command.h"
#include "xfer-completion.h"
#include "xfer-config.h"
#include "xfer-file.h"
#include "xfer-info.h"
#include "xfer-network.h"
#include "xfer-upgrade.h"


WEECHAT_PLUGIN_NAME(XFER_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("DCC file transfer and direct chat"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(XFER_PLUGIN_PRIORITY);

struct t_weechat_plugin *weechat_xfer_plugin = NULL;

char *xfer_type_string[] =             /* strings for types                 */
{ "file_recv_active", "file_recv_passive",
  "file_send_active", "file_send_passive",
  "chat_recv", "chat_send"
};

char *xfer_protocol_string[] =         /* strings for protocols             */
{ "none", "dcc"
};

char *xfer_status_string[] =           /* strings for status                */
{ N_("waiting"), N_("connecting"),
  N_("active"), N_("done"), N_("failed"),
  N_("aborted"), N_("hashing")
};

char *xfer_hash_status_string[] =      /* strings for hash status           */
{ "?",
  N_("CRC in progress"), N_("CRC OK"),
  N_("wrong CRC"), N_("CRC error")
};

struct t_xfer *xfer_list = NULL;       /* list of files/chats               */
struct t_xfer *last_xfer = NULL;       /* last file/chat in list            */
int xfer_count = 0;                    /* number of xfer                    */

int xfer_signal_upgrade_received = 0;  /* signal "upgrade" received ?       */

void xfer_disconnect_all ();


/*
 * Checks if a xfer pointer is valid.
 *
 * Returns:
 *   1: xfer exists
 *   0: xfer does not exist
 */

int
xfer_valid (struct t_xfer *xfer)
{
    struct t_xfer *ptr_xfer;

    if (!xfer)
        return 0;

    for (ptr_xfer = xfer_list; ptr_xfer;
         ptr_xfer = ptr_xfer->next_xfer)
    {
        if (ptr_xfer == xfer)
            return 1;
    }

    /* xfer not found */
    return 0;
}

/*
 * Callback for signal "upgrade".
 */

int
xfer_signal_upgrade_cb (const void *pointer, void *data,
                        const char *signal, const char *type_data,
                        void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    /* only save session and continue? */
    if (signal_data && (strcmp (signal_data, "save") == 0))
    {
        xfer_upgrade_save ();
        return WEECHAT_RC_OK;
    }

    xfer_signal_upgrade_received = 1;

    /*
     * TODO: do not disconnect here in case of upgrade when the save of xfers
     * in upgrade file will be implemented
     * (see function xfer_upgrade_save_xfers in xfer-upgrade.c)
     */
    /*if (signal_data && (strcmp (signal_data, "quit") == 0))*/
    xfer_disconnect_all ();

    return WEECHAT_RC_OK;
}


/*
 * Creates directories for xfer plugin.
 */

void
xfer_create_directories ()
{
    char *path;
    struct t_hashtable *options;

    options = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (options)
        weechat_hashtable_set (options, "directory", "data");

    /* create download directory */
    path = weechat_string_eval_path_home (
        weechat_config_string (xfer_config_file_download_path),
        NULL, NULL, options);
    if (path)
    {
        (void) weechat_mkdir_parents (path, 0700);
        free (path);
    }

    /* create upload directory */
    path = weechat_string_eval_path_home (
        weechat_config_string (xfer_config_file_upload_path),
        NULL, NULL, options);
    if (path)
    {
        (void) weechat_mkdir_parents (path, 0700);
        free (path);
    }

    weechat_hashtable_free (options);
}

/*
 * Searches for xfer type.
 *
 * Returns index of type in enum t_xfer_type, -1 if not found.
 */

int
xfer_search_type (const char *type)
{
    int i;

    if (!type)
        return -1;

    for (i = 0; i < XFER_NUM_TYPES; i++)
    {
        if (strcmp (xfer_type_string[i], type) == 0)
            return i;
    }

    /* xfer type not found */
    return -1;
}

/*
 * Searches for xfer protocol.
 *
 * Returns index of protocol in enum t_xfer_protocol, -1 if not found.
 */

int
xfer_search_protocol (const char *protocol)
{
    int i;

    if (!protocol)
        return -1;

    for (i = 0; i < XFER_NUM_PROTOCOLS; i++)
    {
        if (strcmp (xfer_protocol_string[i], protocol) == 0)
            return i;
    }

    /* xfer protocol not found */
    return -1;
}

/*
 * Searches for a xfer.
 *
 * Returns pointer to xfer found, NULL if not found.
 */

struct t_xfer *
xfer_search (const char *plugin_name, const char *plugin_id,
             enum t_xfer_type type, enum t_xfer_status status, int port)
{
    struct t_xfer *ptr_xfer;

    if (!plugin_name || !plugin_id)
        return NULL;

    for (ptr_xfer = xfer_list; ptr_xfer; ptr_xfer = ptr_xfer->next_xfer)
    {
        if ((strcmp (ptr_xfer->plugin_name, plugin_name) == 0)
            && (strcmp (ptr_xfer->plugin_id, plugin_id) == 0)
            && (ptr_xfer->type == type)
            && (ptr_xfer->status = status)
            && (ptr_xfer->port == port))
            return ptr_xfer;
    }

    /* xfer not found */
    return NULL;
}

/*
 * Searches for a xfer by number (first xfer is 0).
 *
 * Returns pointer to xfer found, NULL if not found.
 */

struct t_xfer *
xfer_search_by_number (int number)
{
    struct t_xfer *ptr_xfer;
    int i;

    i = 0;
    for (ptr_xfer = xfer_list; ptr_xfer; ptr_xfer = ptr_xfer->next_xfer)
    {
        if (i == number)
            return ptr_xfer;
        i++;
    }

    /* xfer not found */
    return NULL;
}

/*
 * Searches for a xfer by buffer (for chat only).
 *
 * Returns pointer to xfer found, NULL if not found.
 */

struct t_xfer *
xfer_search_by_buffer (struct t_gui_buffer *buffer)
{
    struct t_xfer *ptr_xfer;

    if (!buffer)
        return NULL;

    for (ptr_xfer = xfer_list; ptr_xfer; ptr_xfer = ptr_xfer->next_xfer)
    {
        if (ptr_xfer->buffer == buffer)
            return ptr_xfer;
    }

    /* xfer not found */
    return NULL;
}

/*
 * Closes a xfer.
 */

void
xfer_close (struct t_xfer *xfer, enum t_xfer_status status)
{
    struct stat st;

    xfer->status = status;

    if (XFER_HAS_ENDED(xfer->status))
    {
        if (xfer->hook_fd)
        {
            weechat_unhook (xfer->hook_fd);
            xfer->hook_fd = NULL;
        }
        if (xfer->hook_timer)
        {
            weechat_unhook (xfer->hook_timer);
            xfer->hook_timer = NULL;
        }
        if (xfer->hook_connect)
        {
            weechat_unhook (xfer->hook_connect);
            xfer->hook_connect = NULL;
        }
        if (XFER_IS_FILE(xfer->type))
        {
            weechat_printf (NULL,
                            _("%s%s: file %s %s %s (%s): %s"),
                            (xfer->status == XFER_STATUS_DONE) ? "" : weechat_prefix ("error"),
                            XFER_PLUGIN_NAME,
                            xfer->filename,
                            (xfer->type == XFER_TYPE_FILE_SEND_PASSIVE) ? _("sent to") : _("received from"),
                            xfer->remote_nick,
                            xfer->remote_address_str,
                            (xfer->status == XFER_STATUS_DONE) ? _("OK") : _("FAILED"));
            xfer_network_child_kill (xfer);
        }
    }
    if (xfer->status == XFER_STATUS_ABORTED)
    {
        if (XFER_IS_CHAT(xfer->type))
        {
            weechat_printf (xfer->buffer,
                            _("%s%s: chat closed with %s "
                              "(%s)"),
                            weechat_prefix ("network"),
                            XFER_PLUGIN_NAME,
                            xfer->remote_nick,
                            xfer->remote_address_str);
        }
    }

    /* remove empty file if received file failed and nothing was transferred */
    if (((xfer->status == XFER_STATUS_FAILED)
         || (xfer->status == XFER_STATUS_ABORTED))
        && XFER_IS_FILE(xfer->type)
        && XFER_IS_RECV(xfer->type)
        && xfer->temp_local_filename
        && xfer->pos == 0)
    {
        /* erase file only if really empty on disk */
        if (stat (xfer->temp_local_filename, &st) != -1)
        {
            if ((unsigned long long) st.st_size == 0)
                unlink (xfer->temp_local_filename);
        }
    }

    /* rename received file if it has a suffix */
    if ((xfer->status == XFER_STATUS_DONE)
        && XFER_IS_FILE(xfer->type)
        && XFER_IS_RECV(xfer->type)
        && (strcmp (xfer->local_filename, xfer->temp_local_filename) != 0))
    {
        rename (xfer->temp_local_filename, xfer->local_filename);
    }

    if (XFER_IS_FILE(xfer->type))
        xfer_file_calculate_speed (xfer, 1);

    if (xfer->sock >= 0)
    {
        close (xfer->sock);
        xfer->sock = -1;
    }
    if (xfer->file >= 0)
    {
        close (xfer->file);
        xfer->file = -1;
    }

    if (XFER_HAS_ENDED(xfer->status))
        xfer_send_signal (xfer, "xfer_ended");
}

/*
 * Disconnects all active xfer (with a socket).
 */

void
xfer_disconnect_all ()
{
    struct t_xfer *ptr_xfer;

    for (ptr_xfer = xfer_list; ptr_xfer; ptr_xfer = ptr_xfer->next_xfer)
    {
        if (ptr_xfer->sock >= 0)
        {
            if (ptr_xfer->status == XFER_STATUS_ACTIVE)
            {
                weechat_printf (NULL,
                                _("%s%s: aborting active xfer: \"%s\" from %s"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME,
                                ptr_xfer->filename, ptr_xfer->remote_nick);
                weechat_log_printf (_("%s%s: aborting active xfer: \"%s\" from %s"),
                                    "", XFER_PLUGIN_NAME,
                                    ptr_xfer->filename,
                                    ptr_xfer->remote_nick);
            }
            xfer_close (ptr_xfer,
                        (XFER_IS_CHAT(ptr_xfer->type)) ?
                        XFER_STATUS_ABORTED : XFER_STATUS_FAILED);
        }
    }
}

/*
 * Checks if a port is in used.
 *
 * Returns:
 *   1: port is in used (by an active or connecting xfer)
 *   0: port is not in used
 */

int
xfer_port_in_use (int port)
{
    struct t_xfer *ptr_xfer;

    /* skip any currently used ports */
    for (ptr_xfer = xfer_list; ptr_xfer; ptr_xfer = ptr_xfer->next_xfer)
    {
        if ((ptr_xfer->port == port) && (!XFER_HAS_ENDED(ptr_xfer->status)))
            return 1;
    }

    /* port not in use */
    return 0;
}

/*
 * Sends a signal for a xfer.
 */

void
xfer_send_signal (struct t_xfer *xfer, const char *signal)
{
    struct t_infolist *infolist;

    infolist = weechat_infolist_new ();
    if (infolist)
    {
        if (xfer_add_to_infolist (infolist, xfer))
        {
            (void) weechat_hook_signal_send (signal,
                                             WEECHAT_HOOK_SIGNAL_POINTER,
                                             infolist);
        }
        weechat_infolist_free (infolist);
    }
}

/*
 * Allocates a new xfer.
 *
 * Returns pointer to new xfer, NULL if error.
 */

struct t_xfer *
xfer_alloc ()
{
    struct t_xfer *new_xfer;
    time_t time_now;

    /* create new xfer struct */
    if ((new_xfer = malloc (sizeof (*new_xfer))) == NULL)
        return NULL;

    time_now = time (NULL);

    new_xfer->plugin_id = NULL;
    new_xfer->plugin_name = NULL;
    new_xfer->type = 0;
    new_xfer->protocol = 0;
    new_xfer->remote_nick = NULL;
    new_xfer->local_nick = NULL;
    new_xfer->charset_modifier = NULL;
    new_xfer->filename = NULL;
    new_xfer->size = 0;
    new_xfer->proxy = NULL;
    new_xfer->local_address = NULL;
    new_xfer->local_address_length = 0;
    new_xfer->local_address_str = NULL;
    new_xfer->remote_address = NULL;
    new_xfer->remote_address_length = 0;
    new_xfer->remote_address_str = NULL;
    new_xfer->port = 0;
    new_xfer->status = 0;
    new_xfer->buffer = NULL;
    new_xfer->remote_nick_color = NULL;
    new_xfer->fast_send = weechat_config_boolean (xfer_config_network_fast_send);
    new_xfer->send_ack = weechat_config_boolean (xfer_config_network_send_ack);
    new_xfer->blocksize = weechat_config_integer (xfer_config_network_blocksize);
    new_xfer->start_time = time_now;
    new_xfer->start_transfer = time_now;
    new_xfer->sock = -1;
    new_xfer->child_pid = 0;
    new_xfer->child_read = -1;
    new_xfer->child_write = -1;
    new_xfer->hook_fd = NULL;
    new_xfer->hook_timer = NULL;
    new_xfer->hook_connect = NULL;
    new_xfer->unterminated_message = NULL;
    new_xfer->file = -1;
    new_xfer->local_filename = NULL;
    new_xfer->temp_local_filename = NULL;
    new_xfer->filename_suffix = -1;
    new_xfer->pos = 0;
    new_xfer->ack = 0;
    new_xfer->start_resume = 0;
    new_xfer->last_check_time = time_now;
    new_xfer->last_check_pos = time_now;
    new_xfer->last_activity = 0;
    new_xfer->bytes_per_sec = 0;
    new_xfer->eta = 0;
    new_xfer->hash_handle = NULL;
    new_xfer->hash_target = NULL;
    new_xfer->hash_status = XFER_HASH_STATUS_UNKNOWN;

    new_xfer->prev_xfer = NULL;
    new_xfer->next_xfer = xfer_list;
    if (xfer_list)
        xfer_list->prev_xfer = new_xfer;
    else
        last_xfer = new_xfer;
    xfer_list = new_xfer;

    xfer_count++;

    return new_xfer;
}

/*
 * Checks if the given server/nick is auto-accepted.
 *
 * Returns:
 *   1: nick auto-accepted
 *   0: nick not auto-accepted
 */

int
xfer_nick_auto_accepted (const char *server, const char *nick)
{
    int rc, i, num_nicks, num_chars;
    char **nicks, *pos;

    rc = 0;

    nicks = weechat_string_split (
        weechat_config_string (xfer_config_file_auto_accept_nicks),
        ",",
        NULL,
        WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
        0,
        &num_nicks);
    if (nicks)
    {
        for (i = 0; i < num_nicks; i++)
        {
            pos = strrchr (nicks[i], '.');
            if (pos)
            {
                num_chars = weechat_utf8_pos (nicks[i], pos - nicks[i]);
                if ((weechat_strncasecmp (server, nicks[i], num_chars) == 0)
                    && (weechat_strcasecmp (nick, pos + 1) == 0))
                {
                    rc = 1;
                    break;
                }
            }
            else
            {
                if (weechat_strcasecmp (nick, nicks[i]) == 0)
                {
                    rc = 1;
                    break;
                }
            }
        }
        weechat_string_free_split (nicks);
    }

    return rc;
}

/*
 * Adds a xfer to list.
 *
 * Returns pointer to new xfer, NULL if error.
 */

struct t_xfer *
xfer_new (const char *plugin_name, const char *plugin_id,
          enum t_xfer_type type, enum t_xfer_protocol protocol,
          const char *remote_nick, const char *local_nick,
          const char *charset_modifier, const char *filename,
          unsigned long long size, const char *proxy, struct sockaddr *remote_address,
          socklen_t remote_address_length, struct sockaddr *local_address,
          socklen_t local_address_length, int port, int sock,
          const char *local_filename, const char *token)
{
    struct t_xfer *new_xfer;
    const char *ptr_crc32;
    char str_address[NI_MAXHOST], *color;
    int rc;

    new_xfer = xfer_alloc ();
    if (!new_xfer)
    {
        weechat_printf (NULL,
                        _("%s%s: not enough memory for new xfer"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME);
        return NULL;
    }

    if (!xfer_buffer
        && weechat_config_boolean (xfer_config_look_auto_open_buffer))
    {
        xfer_buffer_open ();
    }

    /* initialize new xfer */
    new_xfer->plugin_name = strdup (plugin_name);
    new_xfer->plugin_id = strdup (plugin_id);
    new_xfer->type = type;
    new_xfer->protocol = protocol;
    new_xfer->remote_nick = strdup (remote_nick);
    color = weechat_info_get ("irc_nick_color_name", remote_nick);
    new_xfer->remote_nick_color = (color) ? strdup (color) : NULL;
    free (color);
    new_xfer->local_nick = (local_nick) ? strdup (local_nick) : NULL;
    new_xfer->charset_modifier = (charset_modifier) ? strdup (charset_modifier) : NULL;
    if (XFER_IS_FILE(type))
        new_xfer->filename = (filename) ? strdup (filename) : NULL;
    else
        new_xfer->filename = strdup (_("xfer chat"));
    new_xfer->size = size;
    new_xfer->proxy = (proxy) ? strdup (proxy) : NULL;
    new_xfer->port = port;
    new_xfer->token = (token) ? strdup (token) : NULL;

    if (XFER_IS_RECV(type))
    {
        rc = getnameinfo ((struct sockaddr *)remote_address, remote_address_length, str_address,
                          sizeof (str_address), NULL, 0, NI_NUMERICHOST);
        if (rc != 0)
        {
            weechat_printf (NULL,
                            _("%s%s: unable to interpret address: error %d %s"),
                            weechat_prefix ("error"), XFER_PLUGIN_NAME,
                            rc, gai_strerror (rc));
            snprintf (str_address, sizeof (str_address), "?");
        }
        xfer_set_remote_address (new_xfer, remote_address, remote_address_length, str_address);
    }
    else
    {
        new_xfer->remote_address_str = strdup ("");
    }

    if (XFER_IS_FILE_PASSIVE(type) || (type == XFER_TYPE_CHAT_SEND))
    {
        rc = getnameinfo ((struct sockaddr *)local_address, local_address_length, str_address,
                          sizeof (str_address), NULL, 0, NI_NUMERICHOST);
        if (rc != 0)
        {
            weechat_printf (NULL,
                            _("%s%s: unable to interpret address: error %d %s"),
                            weechat_prefix ("error"), XFER_PLUGIN_NAME,
                            rc, gai_strerror (rc));
            snprintf (str_address, sizeof (str_address), "?");
        }
        xfer_set_local_address (new_xfer, local_address, local_address_length, str_address);
    }
    else
    {
        new_xfer->local_address_str = strdup ("");
    }

    new_xfer->status = XFER_STATUS_WAITING;
    new_xfer->sock = sock;
    if (local_filename)
        new_xfer->local_filename = strdup (local_filename);
    else
        xfer_file_find_filename (new_xfer);

    new_xfer->hash_handle = NULL;
    new_xfer->hash_target = NULL;
    new_xfer->hash_status = XFER_HASH_STATUS_UNKNOWN;

    if ((type == XFER_TYPE_FILE_RECV_ACTIVE)
        && weechat_config_boolean (xfer_config_file_auto_check_crc32))
    {
        ptr_crc32 = xfer_file_search_crc32 (new_xfer->filename);
        if (ptr_crc32)
        {
            new_xfer->hash_handle = malloc (sizeof (gcry_md_hd_t));
            if (new_xfer->hash_handle)
            {
                if (gcry_md_open (new_xfer->hash_handle, GCRY_MD_CRC32, 0) == 0)
                {
                    new_xfer->hash_target = weechat_strndup (ptr_crc32, 8);
                    new_xfer->hash_status = XFER_HASH_STATUS_IN_PROGRESS;
                }
                else
                {
                    free (new_xfer->hash_handle);
                    new_xfer->hash_handle = NULL;
                    weechat_printf (NULL,
                                    _("%s%s: hashing error"),
                                    weechat_prefix ("error"), XFER_PLUGIN_NAME);
                }
            }
        }
    }

    /* write info message on core buffer */
    switch (type)
    {
        case XFER_TYPE_FILE_RECV_ACTIVE:
        case XFER_TYPE_FILE_RECV_PASSIVE:
            weechat_printf (NULL,
                            _("%s: incoming file from %s "
                              "(%s, %s.%s), name: %s, %llu bytes "
                              "(protocol: %s)"),
                            XFER_PLUGIN_NAME,
                            remote_nick,
                            new_xfer->remote_address_str,
                            plugin_name,
                            plugin_id,
                            filename,
                            size,
                            xfer_protocol_string[protocol]);
            xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
            break;
        case XFER_TYPE_FILE_SEND_ACTIVE:
        case XFER_TYPE_FILE_SEND_PASSIVE:
            weechat_printf (NULL,
                            _("%s: offering file to %s (%s.%s), name: %s "
                              "(local filename: %s), %llu bytes (protocol: %s)"),
                            XFER_PLUGIN_NAME,
                            remote_nick,
                            plugin_name,
                            plugin_id,
                            filename,
                            local_filename,
                            size,
                            xfer_protocol_string[protocol]);
            xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
            break;
        case XFER_TYPE_CHAT_RECV:
            weechat_printf (NULL,
                            _("%s: incoming chat request from %s "
                              "(%s, %s.%s)"),
                            XFER_PLUGIN_NAME,
                            remote_nick,
                            new_xfer->remote_address_str,
                            plugin_name,
                            plugin_id);
            xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
            break;
        case XFER_TYPE_CHAT_SEND:
            weechat_printf (NULL,
                            _("%s: sending chat request to %s (%s.%s)"),
                            XFER_PLUGIN_NAME,
                            remote_nick,
                            plugin_name,
                            plugin_id);
            xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
            break;
        case XFER_NUM_TYPES:
            break;
    }

    if (XFER_IS_FILE(type) && (!new_xfer->local_filename))
    {
        xfer_close (new_xfer, XFER_STATUS_FAILED);
        xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
        return NULL;
    }

    if (XFER_IS_FILE(type) && (new_xfer->start_resume > 0))
    {
        weechat_printf (NULL,
                        _("%s: file %s (local filename: %s) "
                          "will be resumed at position %llu"),
                        XFER_PLUGIN_NAME,
                        new_xfer->filename,
                        new_xfer->local_filename,
                        new_xfer->start_resume);
        xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
    }

    /* connect if needed and display again xfer buffer */
    if (XFER_IS_SEND(type))
    {
        if (!xfer_network_connect (new_xfer))
        {
            xfer_close (new_xfer, XFER_STATUS_FAILED);
            xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
            return NULL;
        }
    }

    /*
     * auto-accept file/chat if nick is auto-accepted, or if file/chat is
     * auto-accepted
     */
    if ((XFER_IS_RECV(type)
         && xfer_nick_auto_accepted (new_xfer->plugin_id, new_xfer->remote_nick))
        || ((type == XFER_TYPE_FILE_RECV_ACTIVE)
            && weechat_config_boolean (xfer_config_file_auto_accept_files))
        || ((type == XFER_TYPE_CHAT_RECV)
            && weechat_config_boolean (xfer_config_file_auto_accept_chats)))
    {
        xfer_network_accept (new_xfer);
    }
    else
    {
        xfer_buffer_refresh (WEECHAT_HOTLIST_PRIVATE);
    }

    return new_xfer;
}

/*
 * Sets the remote address field.
 */

void
xfer_set_remote_address (struct t_xfer *xfer, const struct sockaddr *address,
                         socklen_t length, const char *address_str)
{
    free (xfer->remote_address);
    xfer->remote_address = malloc (length);
    xfer->remote_address_length = length;
    if (xfer->remote_address)
	memcpy (xfer->remote_address, address, length);

    free (xfer->remote_address_str);
    xfer->remote_address_str = strdup ((address_str) ? address_str : "");
}

/*
 * Sets the local address field.
 */
void
xfer_set_local_address (struct t_xfer *xfer, const struct sockaddr *address,
                         socklen_t length, const char *address_str)
{
    free (xfer->local_address);
    xfer->local_address = malloc (length);
    xfer->local_address_length = length;
    if (xfer->local_address)
	memcpy (xfer->local_address, address, length);

    free (xfer->local_address_str);
    xfer->local_address_str = strdup ((address_str) ? address_str : "");
}

/*
 * Frees xfer struct and removes it from list.
 */

void
xfer_free (struct t_xfer *xfer)
{
    struct t_xfer *new_xfer_list;

    if (!xfer)
        return;

    /* remove xfer from list */
    if (last_xfer == xfer)
        last_xfer = xfer->prev_xfer;
    if (xfer->prev_xfer)
    {
        (xfer->prev_xfer)->next_xfer = xfer->next_xfer;
        new_xfer_list = xfer_list;
    }
    else
        new_xfer_list = xfer->next_xfer;
    if (xfer->next_xfer)
        (xfer->next_xfer)->prev_xfer = xfer->prev_xfer;

    /* free data */
    free (xfer->plugin_id);
    free (xfer->plugin_name);
    free (xfer->remote_nick);
    free (xfer->local_nick);
    free (xfer->charset_modifier);
    free (xfer->filename);
    free (xfer->proxy);
    free (xfer->local_address);
    free (xfer->local_address_str);
    free (xfer->remote_address);
    free (xfer->remote_address_str);
    free (xfer->remote_nick_color);
    weechat_unhook (xfer->hook_fd);
    weechat_unhook (xfer->hook_timer);
    weechat_unhook (xfer->hook_connect);
    free (xfer->unterminated_message);
    free (xfer->local_filename);
    free (xfer->temp_local_filename);
    if (xfer->hash_handle)
    {
        gcry_md_close (*xfer->hash_handle);
        free (xfer->hash_handle);
    }
    free (xfer->hash_target);

    free (xfer);

    xfer_list = new_xfer_list;

    xfer_count--;
    if (xfer_buffer_selected_line >= xfer_count)
        xfer_buffer_selected_line = (xfer_count == 0) ? 0 : xfer_count - 1;
}

/*
 * Frees all xfers.
 */

void
xfer_free_all ()
{
    while (xfer_list)
    {
        xfer_free (xfer_list);
    }
}

/*
 * Callback for signal "xfer_add".
 */

int
xfer_add_cb (const void *pointer, void *data,
             const char *signal, const char *type_data,
             void *signal_data)
{
    struct t_infolist *infolist;
    const char *plugin_name, *plugin_id, *str_type, *str_protocol;
    const char *remote_nick, *local_nick, *charset_modifier, *filename, *proxy;
    const char *str_address, *str_port, *token;
    int type, protocol, args, port_start, port_end, sock, server_sock, port;
    char *path, *filename2, *short_filename, *pos, str_port_temp[16];
    struct stat st;
    struct sockaddr_storage local_addr_storage, remote_addr_storage, own_ip_addr, bind_addr;
    struct sockaddr *local_addr = (struct sockaddr*)&local_addr_storage;
    struct sockaddr *remote_addr = (struct sockaddr*)&remote_addr_storage;
    socklen_t local_addr_length, remote_addr_length, bind_addr_len;
    unsigned long long file_size;
    struct t_xfer *ptr_xfer;
    struct t_hashtable *options;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data)
    {
        weechat_printf (NULL,
                        _("%s%s: missing arguments (%s)"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME, "xfer_add");
        return WEECHAT_RC_ERROR;
    }

    infolist = (struct t_infolist *)signal_data;

    if (!weechat_infolist_next (infolist))
    {
        weechat_printf (NULL,
                        _("%s%s: missing arguments (%s)"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME, "xfer_add");
        return WEECHAT_RC_ERROR;
    }

    filename2 = NULL;
    short_filename = NULL;
    file_size = 0;

    sock = -1;

    plugin_name = weechat_infolist_string (infolist, "plugin_name");
    plugin_id = weechat_infolist_string (infolist, "plugin_id");
    str_type = weechat_infolist_string (infolist, "type_string");
    str_protocol = weechat_infolist_string (infolist, "protocol_string");
    remote_nick = weechat_infolist_string (infolist, "remote_nick");
    local_nick = weechat_infolist_string (infolist, "local_nick");
    charset_modifier = weechat_infolist_string (infolist, "charset_modifier");
    filename = weechat_infolist_string (infolist, "filename");
    proxy = weechat_infolist_string (infolist, "proxy");
    token = weechat_infolist_string (infolist, "token");
    protocol = XFER_NO_PROTOCOL;

    memset (&local_addr_storage, 0, sizeof (local_addr_storage));
    local_addr_length = sizeof (local_addr_storage);

    if (!plugin_name || !plugin_id || !str_type || !remote_nick || !local_nick)
    {
        weechat_printf (NULL,
                        _("%s%s: missing arguments (%s)"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME, "xfer_add");
        goto error;
    }

    type = xfer_search_type (str_type);
    if (type < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: unknown xfer type \"%s\""),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME, str_type);
        goto error;
    }

    if (XFER_IS_FILE(type) && (!filename || !str_protocol))
    {
        weechat_printf (NULL,
                        _("%s%s: missing arguments (%s)"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME, "xfer_add");
        goto error;
    }

    if (XFER_IS_FILE(type))
    {
        protocol = xfer_search_protocol (str_protocol);
        if (protocol < 0)
        {
            weechat_printf (NULL,
                            _("%s%s: unknown xfer protocol \"%s\""),
                            weechat_prefix ("error"), XFER_PLUGIN_NAME,
                            str_protocol);
            goto error;
        }
    }

    if ((type == XFER_TYPE_FILE_RECV_ACTIVE)
        || (type == XFER_TYPE_FILE_RECV_PASSIVE))
    {
        filename2 = strdup (filename);
        sscanf (weechat_infolist_string (infolist, "size"), "%llu", &file_size);
    }

    if (type == XFER_TYPE_FILE_SEND_PASSIVE)
    {
        /* add home if filename not beginning with '/' or '~' (not for Win32) */
#ifdef _WIN32
        filename2 = strdup (filename);
#else
        if (filename[0] == '/')
            filename2 = strdup (filename);
        else if (filename[0] == '~')
            filename2 = weechat_string_expand_home (filename);
        else
        {
            options = weechat_hashtable_new (
                32,
                WEECHAT_HASHTABLE_STRING,
                WEECHAT_HASHTABLE_STRING,
                NULL, NULL);
            if (options)
                weechat_hashtable_set (options, "directory", "data");
            path = weechat_string_eval_path_home (
                weechat_config_string (xfer_config_file_upload_path),
                NULL, NULL, options);
            weechat_hashtable_free (options);
            if (!path)
            {
                weechat_printf (NULL,
                                _("%s%s: not enough memory (%s)"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME,
                                "xfer_add, path");
                goto error;
            }
            filename2 = malloc (strlen (path) + strlen (filename) + 4);
            if (!filename2)
            {
                weechat_printf (NULL,
                                _("%s%s: not enough memory (%s)"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME,
                                "xfer_add, filename2");
                free (path);
                goto error;
            }
            strcpy (filename2, path);
            if (filename2[strlen (filename2) - 1] != DIR_SEPARATOR_CHAR)
                strcat (filename2, DIR_SEPARATOR);
            strcat (filename2, filename);
            free (path);
        }
#endif /* _WIN32 */
        if (!filename2)
        {
            weechat_printf (NULL,
                            _("%s%s: not enough memory (%s)"),
                            weechat_prefix ("error"), XFER_PLUGIN_NAME,
                            "xfer_add, filename2");
            goto error;
        }
        /* check if file exists */
        if (stat (filename2, &st) == -1)
        {
            weechat_printf (NULL,
                            _("%s%s: cannot access file \"%s\""),
                            weechat_prefix ("error"), XFER_PLUGIN_NAME,
                            filename2);
            goto error;
        }
        file_size = st.st_size;
    }
    port = weechat_infolist_integer (infolist, "port");

    /* resolve address */
    if ((type == XFER_TYPE_CHAT_RECV) || (type == XFER_TYPE_FILE_RECV_ACTIVE))
    {
        str_address = weechat_infolist_string (infolist, "remote_address");
        snprintf (str_port_temp, sizeof (str_port_temp), "%d", port);
        str_port = str_port_temp;
        remote_addr_length = sizeof (remote_addr_storage);
        if (!xfer_network_resolve_addr (str_address, str_port,
                                        (struct sockaddr*)&remote_addr_storage, &remote_addr_length,
                                        AI_NUMERICSERV | AI_NUMERICHOST))
        {
            goto error;
        }
    }
    if (type == XFER_TYPE_FILE_RECV_PASSIVE)
    {
        str_address = weechat_infolist_string (infolist, "remote_address");
        remote_addr_length = sizeof (remote_addr_storage);
        if (!xfer_network_resolve_addr (str_address, NULL,
                                        (struct sockaddr*)&remote_addr_storage, &remote_addr_length,
                                        AI_NUMERICSERV | AI_NUMERICHOST))
        {
            goto error;
        }
    }
    if (XFER_IS_FILE_PASSIVE(type) || (type == XFER_TYPE_CHAT_SEND))
    {
        memset (&bind_addr, 0, sizeof (bind_addr));

        /* determine bind_addr family from either own_ip or default */
        if (weechat_config_string (xfer_config_network_own_ip)
            && weechat_config_string (xfer_config_network_own_ip)[0])
        {
            /* resolve own_ip to a numeric address */
            str_address = weechat_config_string (xfer_config_network_own_ip);
            local_addr_length = sizeof (own_ip_addr);

            if (!xfer_network_resolve_addr (str_address, NULL,
                                            (struct sockaddr*)&own_ip_addr,
                                            &local_addr_length,
                                            AI_NUMERICSERV))
            {
                goto error;
            }

            /* set the advertised address to own_ip */
            local_addr = (struct sockaddr*)&own_ip_addr;

            /* bind_addr's family should be the advertised family */
            bind_addr.ss_family = own_ip_addr.ss_family;
        }
        else
        {
            /* no own_ip, so bind_addr's family comes from irc connection  */
            /* use the local interface, from the server socket */
            server_sock = weechat_infolist_integer (infolist, "socket");
            if (getsockname (server_sock, (struct sockaddr *)&local_addr_storage, &local_addr_length))
            {
                goto error;
            }
            bind_addr.ss_family = local_addr_storage.ss_family;
        }

        /* determine bind wildcard address */
        if (bind_addr.ss_family == AF_INET)
        {
            ((struct sockaddr_in*)&bind_addr)->sin_addr.s_addr = INADDR_ANY;
            bind_addr_len = sizeof (struct sockaddr_in);
        }
        else
        {
            memcpy (&((struct sockaddr_in6*)&bind_addr)->sin6_addr,
                    &in6addr_any, sizeof (in6addr_any));
            bind_addr_len = sizeof (struct sockaddr_in6);
        }

        /* open socket for xfer */
        sock = socket (bind_addr.ss_family, SOCK_STREAM, 0);
        if (sock < 0)
        {
            weechat_printf (NULL,
                            _("%s%s: cannot create socket for xfer: error %d %s"),
                            weechat_prefix ("error"), XFER_PLUGIN_NAME,
                            errno, strerror (errno));
            goto error;
        }

        /* look for port */
        port = 0;

        if (weechat_config_string (xfer_config_network_port_range)
            && weechat_config_string (xfer_config_network_port_range)[0])
        {
            /* find a free port in the specified range */
            args = sscanf (weechat_config_string (xfer_config_network_port_range),
                           "%d-%d", &port_start, &port_end);
            if (args > 0)
            {
                port = port_start;
                if (args == 1)
                    port_end = port_start;

                /* loop through the entire allowed port range */
                while (port <= port_end)
                {
                    if (!xfer_port_in_use (port))
                    {
                        /* attempt to bind to the free port */
                        if (bind_addr.ss_family == AF_INET)
                            ((struct sockaddr_in *)&bind_addr)->sin_port = htons (port);
                        else
                            ((struct sockaddr_in6 *)&bind_addr)->sin6_port = htons (port);
                        if (bind (sock, (struct sockaddr *)&bind_addr, bind_addr_len) == 0)
                            break;
                    }
                    port++;
                }

                if (port > port_end)
                    port = -1;
            }
        }

        if (port == 0)
        {
            /* find port automatically */
            if (bind (sock, (struct sockaddr *)&bind_addr, bind_addr_len) == 0)
            {
                getsockname (sock, (struct sockaddr *)&bind_addr, &bind_addr_len);
                if (bind_addr.ss_family == AF_INET)
                    port = ntohs (((struct sockaddr_in *)&bind_addr)->sin_port);
                else
                    port = ntohs (((struct sockaddr_in6 *)&bind_addr)->sin6_port);
            }
            else
                port = -1;
        }

        if (port == -1)
        {
            /* Could not find any port to bind */
            weechat_printf (NULL,
                            _("%s%s: cannot find available port for xfer"),
                            weechat_prefix ("error"), XFER_PLUGIN_NAME);
            close (sock);
            goto error;
        }
    }

    if (XFER_IS_FILE(type) && filename2)
    {
        /* extract short filename (without path) */
        pos = strrchr (filename2, DIR_SEPARATOR_CHAR);
        if (pos)
            short_filename = strdup (pos + 1);
        else
            short_filename = strdup (filename2);

        /* convert spaces to underscore if asked and needed */
        pos = short_filename;
        while (pos[0])
        {
            if (pos[0] == ' ')
            {
                if (weechat_config_boolean (xfer_config_file_convert_spaces))
                    pos[0] = '_';
            }
            pos++;
        }
    }

    if ((type == XFER_TYPE_FILE_RECV_ACTIVE)
        || (type == XFER_TYPE_FILE_RECV_PASSIVE))
    {
        if (filename2)
        {
            free (filename2);
            filename2 = NULL;
        }
    }

    /* add xfer entry and listen to socket if type is file or chat "send" */
    if (XFER_IS_FILE(type))
    {
        ptr_xfer = xfer_new (plugin_name, plugin_id, type, protocol,
                             remote_nick, local_nick, charset_modifier,
                             short_filename, file_size, proxy,
                             remote_addr, remote_addr_length,
                             local_addr, local_addr_length,
                             port, sock, filename2, token);
    }
    else
    {
        ptr_xfer = xfer_new (plugin_name, plugin_id, type, protocol,
                             remote_nick, local_nick, charset_modifier,
                             NULL, 0, proxy,
                             remote_addr, remote_addr_length,
                             local_addr, local_addr_length,
                             port, sock, NULL, token);
    }

    if (!ptr_xfer)
    {
        weechat_printf (NULL,
                        _("%s%s: error creating xfer"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME);
        close (sock);
        goto error;
    }

    /* send signal if type is file or chat "send" */
    if (XFER_IS_SEND(ptr_xfer->type) && !XFER_HAS_ENDED(ptr_xfer->status))
        xfer_send_signal (ptr_xfer, "xfer_send_ready");

    free (filename2);
    free (short_filename);
    weechat_infolist_reset_item_cursor (infolist);
    return WEECHAT_RC_OK_EAT;

error:
    free (filename2);
    free (short_filename);
    weechat_infolist_reset_item_cursor (infolist);
    return WEECHAT_RC_ERROR;
}

/*
 * Callback called when resume is accepted by sender.
 */

int
xfer_start_resume_cb (const void *pointer, void *data,
                      const char *signal, const char *type_data,
                      void *signal_data)
{
    struct t_infolist *infolist;
    struct t_xfer *ptr_xfer;
    const char *plugin_name, *plugin_id, *filename, *str_start_resume, *token;
    int port;
    unsigned long long start_resume;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data)
    {
        weechat_printf (NULL,
                        _("%s%s: missing arguments (%s)"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME,
                        "xfer_start_resume");
        return WEECHAT_RC_ERROR;
    }

    infolist = (struct t_infolist *)signal_data;

    if (!weechat_infolist_next (infolist))
    {
        weechat_printf (NULL,
                        _("%s%s: missing arguments (%s)"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME,
                        "xfer_start_resume");
        return WEECHAT_RC_ERROR;
    }

    plugin_name = weechat_infolist_string (infolist, "plugin_name");
    plugin_id = weechat_infolist_string (infolist, "plugin_id");
    filename = weechat_infolist_string (infolist, "filename");
    port = weechat_infolist_integer (infolist, "port");
    str_start_resume = weechat_infolist_string (infolist, "start_resume");
    token = weechat_infolist_string (infolist, "token");

    if (!plugin_name || !plugin_id || !filename || !str_start_resume)
    {
        weechat_printf (NULL,
                        _("%s%s: missing arguments (%s)"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME,
                        "xfer_start_resume");
        goto error;
    }

    sscanf (str_start_resume, "%llu", &start_resume);

    ptr_xfer = xfer_search (plugin_name, plugin_id,
                            (!token) ? XFER_TYPE_FILE_RECV_ACTIVE : XFER_TYPE_FILE_RECV_PASSIVE,
                            XFER_STATUS_CONNECTING, port);
    if (ptr_xfer)
    {
        ptr_xfer->pos = start_resume;
        ptr_xfer->ack = start_resume;
        ptr_xfer->start_resume = start_resume;
        ptr_xfer->last_check_pos = start_resume;
        xfer_network_connect_init (ptr_xfer);
    }
    else
    {
        weechat_printf (NULL,
                        _("%s%s: unable to resume file \"%s\" (port: %d, "
                          "start position: %llu): xfer not found or not ready "
                          "for transfer"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME, filename,
                        port, start_resume);
    }

    weechat_infolist_reset_item_cursor (infolist);
    return WEECHAT_RC_OK;

error:
    weechat_infolist_reset_item_cursor (infolist);
    return WEECHAT_RC_ERROR;
}

/*
 * Callback called when sender receives resume request from receiver.
 */

int
xfer_accept_resume_cb (const void *pointer, void *data,
                       const char *signal, const char *type_data,
                       void *signal_data)
{
    struct t_infolist *infolist;
    struct t_xfer *ptr_xfer;
    const char *plugin_name, *plugin_id, *filename, *str_start_resume;
    int port;
    unsigned long long start_resume;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data)
    {
        weechat_printf (NULL,
                        _("%s%s: missing arguments (%s)"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME,
                        "xfer_accept_resume");
        return WEECHAT_RC_ERROR;
    }

    infolist = (struct t_infolist *)signal_data;

    if (!weechat_infolist_next (infolist))
    {
        weechat_printf (NULL,
                        _("%s%s: missing arguments (%s)"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME,
                        "xfer_accept_resume");
        return WEECHAT_RC_ERROR;
    }

    plugin_name = weechat_infolist_string (infolist, "plugin_name");
    plugin_id = weechat_infolist_string (infolist, "plugin_id");
    filename = weechat_infolist_string (infolist, "filename");
    port = weechat_infolist_integer (infolist, "port");
    str_start_resume = weechat_infolist_string (infolist, "start_resume");

    if (!plugin_name || !plugin_id || !filename || !str_start_resume)
    {
        weechat_printf (NULL,
                        _("%s%s: missing arguments (%s)"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME,
                        "xfer_accept_resume");
        goto error;
    }

    sscanf (str_start_resume, "%llu", &start_resume);

    ptr_xfer = xfer_search (plugin_name, plugin_id, XFER_TYPE_FILE_SEND_PASSIVE,
                            XFER_STATUS_CONNECTING, port);
    if (ptr_xfer)
    {
        ptr_xfer->pos = start_resume;
        ptr_xfer->ack = start_resume;
        ptr_xfer->start_resume = start_resume;
        ptr_xfer->last_check_pos = start_resume;
        xfer_send_signal (ptr_xfer, "xfer_send_accept_resume");

        weechat_printf (NULL,
                        _("%s: file %s resumed at position %llu"),
                        XFER_PLUGIN_NAME,
                        ptr_xfer->filename,
                        ptr_xfer->start_resume);
        xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
    }
    else
    {
        weechat_printf (NULL,
                        _("%s%s: unable to accept resume file \"%s\" (port: %d, "
                          "start position: %llu): xfer not found or not ready "
                          "for transfer"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME, filename,
                        port, start_resume);
    }

    weechat_infolist_reset_item_cursor (infolist);
    return WEECHAT_RC_OK;

error:
    weechat_infolist_reset_item_cursor (infolist);
    return WEECHAT_RC_ERROR;
}

/*
 * Adds a xfer in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
xfer_add_to_infolist (struct t_infolist *infolist, struct t_xfer *xfer)
{
    struct t_infolist_item *ptr_item;
    char value[128];

    if (!infolist || !xfer)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_string (ptr_item, "plugin_name", xfer->plugin_name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "plugin_id", xfer->plugin_id))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "type", xfer->type))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "type_string", xfer_type_string[xfer->type]))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "protocol", xfer->protocol))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "protocol_string", xfer_protocol_string[xfer->protocol]))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "remote_nick", xfer->remote_nick))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "local_nick", xfer->local_nick))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "charset_modifier", xfer->charset_modifier))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "filename", xfer->filename))
        return 0;
    snprintf (value, sizeof (value), "%llu", xfer->size);
    if (!weechat_infolist_new_var_string (ptr_item, "size", value))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "proxy", xfer->proxy))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "local_address", xfer->local_address_str))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "remote_address", xfer->remote_address_str))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "port", xfer->port))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "token", xfer->token))
        return 0;

    if (!weechat_infolist_new_var_integer (ptr_item, "status", xfer->status))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "status_string", xfer_status_string[xfer->status]))
        return 0;
    if (!weechat_infolist_new_var_pointer (ptr_item, "buffer", xfer->buffer))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "remote_nick_color", xfer->remote_nick_color))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "fast_send", xfer->fast_send))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "blocksize", xfer->blocksize))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "start_time", xfer->start_time))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "start_transfer", xfer->start_transfer))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "sock", xfer->sock))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "child_pid", xfer->child_pid))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "child_read", xfer->child_read))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "child_write", xfer->child_write))
        return 0;
    if (!weechat_infolist_new_var_pointer (ptr_item, "hook_fd", xfer->hook_fd))
        return 0;
    if (!weechat_infolist_new_var_pointer (ptr_item, "hook_timer", xfer->hook_timer))
        return 0;
    if (!weechat_infolist_new_var_pointer (ptr_item, "hook_connect", xfer->hook_connect))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "unterminated_message", xfer->unterminated_message))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "file", xfer->file))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "local_filename", xfer->local_filename))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "temp_local_filename", xfer->temp_local_filename))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "filename_suffix", xfer->filename_suffix))
        return 0;
    snprintf (value, sizeof (value), "%llu", xfer->pos);
    if (!weechat_infolist_new_var_string (ptr_item, "pos", value))
        return 0;
    snprintf (value, sizeof (value), "%llu", xfer->ack);
    if (!weechat_infolist_new_var_string (ptr_item, "ack", value))
        return 0;
    snprintf (value, sizeof (value), "%llu", xfer->start_resume);
    if (!weechat_infolist_new_var_string (ptr_item, "start_resume", value))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "last_check_time", xfer->last_check_time))
        return 0;
    snprintf (value, sizeof (value), "%llu", xfer->last_check_pos);
    if (!weechat_infolist_new_var_string (ptr_item, "last_check_pos", value))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "last_activity", xfer->last_activity))
        return 0;
    snprintf (value, sizeof (value), "%llu", xfer->bytes_per_sec);
    if (!weechat_infolist_new_var_string (ptr_item, "bytes_per_sec", value))
        return 0;
    snprintf (value, sizeof (value), "%llu", xfer->eta);
    if (!weechat_infolist_new_var_string (ptr_item, "eta", value))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "hash_target", xfer->hash_target))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "hash_status", xfer->hash_status))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "hash_status_string", xfer_hash_status_string[xfer->hash_status]))
        return 0;

    return 1;
}

/*
 * Prints xfer infos in WeeChat log file (usually for crash dump).
 */

void
xfer_print_log ()
{
    struct t_xfer *ptr_xfer;

    for (ptr_xfer = xfer_list; ptr_xfer; ptr_xfer = ptr_xfer->next_xfer)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[xfer (addr:%p)]", ptr_xfer);
        weechat_log_printf ("  plugin_name . . . . . . : '%s'", ptr_xfer->plugin_name);
        weechat_log_printf ("  plugin_id . . . . . . . : '%s'", ptr_xfer->plugin_id);
        weechat_log_printf ("  type. . . . . . . . . . : %d (%s)",
                            ptr_xfer->type,
                            xfer_type_string[ptr_xfer->type]);
        weechat_log_printf ("  protocol. . . . . . . . : %d (%s)",
                            ptr_xfer->protocol,
                            xfer_protocol_string[ptr_xfer->protocol]);
        weechat_log_printf ("  remote_nick . . . . . . : '%s'", ptr_xfer->remote_nick);
        weechat_log_printf ("  local_nick. . . . . . . : '%s'", ptr_xfer->local_nick);
        weechat_log_printf ("  charset_modifier. . . . : '%s'", ptr_xfer->charset_modifier);
        weechat_log_printf ("  filename. . . . . . . . : '%s'", ptr_xfer->filename);
        weechat_log_printf ("  size. . . . . . . . . . : %llu", ptr_xfer->size);
        weechat_log_printf ("  proxy . . . . . . . . . : '%s'", ptr_xfer->proxy);
        weechat_log_printf ("  local_address . . . . . : %p", ptr_xfer->local_address);
        weechat_log_printf ("  local_address_length. . : %d", ptr_xfer->local_address_length);
        weechat_log_printf ("  local_address_str . . . : '%s'", ptr_xfer->local_address_str);
        weechat_log_printf ("  remote_address. . . . . : %p", ptr_xfer->remote_address);
        weechat_log_printf ("  remote_address_length . : %d", ptr_xfer->remote_address_length);
        weechat_log_printf ("  remote_address_str. . . : '%s'", ptr_xfer->remote_address_str);
        weechat_log_printf ("  port. . . . . . . . . . : %d", ptr_xfer->port);
        weechat_log_printf ("  token . . . . . . . . . : %s", ptr_xfer->token);

        weechat_log_printf ("  status. . . . . . . . . : %d (%s)",
                            ptr_xfer->status,
                            xfer_status_string[ptr_xfer->status]);
        weechat_log_printf ("  buffer. . . . . . . . . : %p", ptr_xfer->buffer);
        weechat_log_printf ("  remote_nick_color . . . : '%s'", ptr_xfer->remote_nick_color);
        weechat_log_printf ("  fast_send . . . . . . . : %d", ptr_xfer->fast_send);
        weechat_log_printf ("  blocksize . . . . . . . : %d", ptr_xfer->blocksize);
        weechat_log_printf ("  start_time. . . . . . . : %lld", (long long)ptr_xfer->start_time);
        weechat_log_printf ("  start_transfer. . . . . : %lld", (long long)ptr_xfer->start_transfer);
        weechat_log_printf ("  sock. . . . . . . . . . : %d", ptr_xfer->sock);
        weechat_log_printf ("  child_pid . . . . . . . : %d", ptr_xfer->child_pid);
        weechat_log_printf ("  child_read. . . . . . . : %d", ptr_xfer->child_read);
        weechat_log_printf ("  child_write . . . . . . : %d", ptr_xfer->child_write);
        weechat_log_printf ("  hook_fd . . . . . . . . : %p", ptr_xfer->hook_fd);
        weechat_log_printf ("  hook_timer. . . . . . . : %p", ptr_xfer->hook_timer);
        weechat_log_printf ("  hook_connect. . . . . . : %p", ptr_xfer->hook_connect);
        weechat_log_printf ("  unterminated_message. . : '%s'", ptr_xfer->unterminated_message);
        weechat_log_printf ("  file. . . . . . . . . . : %d", ptr_xfer->file);
        weechat_log_printf ("  local_filename. . . . . : '%s'", ptr_xfer->local_filename);
        weechat_log_printf ("  temp_local_filename . . : '%s'", ptr_xfer->temp_local_filename);
        weechat_log_printf ("  filename_suffix . . . . : %d", ptr_xfer->filename_suffix);
        weechat_log_printf ("  pos . . . . . . . . . . : %llu", ptr_xfer->pos);
        weechat_log_printf ("  ack . . . . . . . . . . : %llu", ptr_xfer->ack);
        weechat_log_printf ("  start_resume. . . . . . : %llu", ptr_xfer->start_resume);
        weechat_log_printf ("  last_check_time . . . . : %lld", (long long)ptr_xfer->last_check_time);
        weechat_log_printf ("  last_check_pos. . . . . : %llu", ptr_xfer->last_check_pos);
        weechat_log_printf ("  last_activity . . . . . : %lld", (long long)ptr_xfer->last_activity);
        weechat_log_printf ("  bytes_per_sec . . . . . : %llu", ptr_xfer->bytes_per_sec);
        weechat_log_printf ("  eta . . . . . . . . . . : %llu", ptr_xfer->eta);
        weechat_log_printf ("  hash_target . . . . . . : '%s'", ptr_xfer->hash_target);
        weechat_log_printf ("  hash_status . . . . . . : %d (%s)",
                            ptr_xfer->hash_status,
                            xfer_hash_status_string[ptr_xfer->hash_status]);
        weechat_log_printf ("  prev_xfer . . . . . . . : %p", ptr_xfer->prev_xfer);
        weechat_log_printf ("  next_xfer . . . . . . . : %p", ptr_xfer->next_xfer);
    }
}

/*
 * Callback for signal "debug_dump".
 */

int
xfer_debug_dump_cb (const void *pointer, void *data,
                    const char *signal, const char *type_data,
                    void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data || (strcmp ((char *)signal_data, XFER_PLUGIN_NAME) == 0))
    {
        weechat_log_printf ("");
        weechat_log_printf ("***** \"%s\" plugin dump *****",
                            weechat_plugin->name);

        xfer_print_log ();

        weechat_log_printf ("");
        weechat_log_printf ("***** End of \"%s\" plugin dump *****",
                            weechat_plugin->name);
    }

    return WEECHAT_RC_OK;
}

/*
 * Initializes xfer plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_plugin = plugin;

    xfer_signal_upgrade_received = 0;

    if (!xfer_config_init ())
        return WEECHAT_RC_ERROR;

    xfer_config_read ();

    xfer_create_directories ();

    xfer_command_init ();

    /* hook some signals */
    weechat_hook_signal ("upgrade",
                         &xfer_signal_upgrade_cb, NULL, NULL);
    weechat_hook_signal ("xfer_add",
                         &xfer_add_cb, NULL, NULL);
    weechat_hook_signal ("xfer_start_resume",
                         &xfer_start_resume_cb, NULL, NULL);
    weechat_hook_signal ("xfer_accept_resume",
                         &xfer_accept_resume_cb, NULL, NULL);
    weechat_hook_signal ("debug_dump",
                         &xfer_debug_dump_cb, NULL, NULL);

    /* hook completions */
    xfer_completion_init ();

    xfer_info_init ();

    if (weechat_xfer_plugin->upgrading)
        xfer_upgrade_load ();

    return WEECHAT_RC_OK;
}

/*
 * Ends xfer plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    if (xfer_buffer)
    {
        weechat_buffer_close (xfer_buffer);
        xfer_buffer = NULL;
    }
    xfer_buffer_selected_line = 0;

    xfer_config_write ();

    if (xfer_signal_upgrade_received)
        xfer_upgrade_save ();

    xfer_disconnect_all ();

    xfer_free_all ();

    weechat_config_free (xfer_config_file);
    xfer_config_file = NULL;

    return WEECHAT_RC_OK;
}
