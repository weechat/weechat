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
 * xfer.c: file transfer and direct chat plugin for WeeChat
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

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
WEECHAT_PLUGIN_AUTHOR("Sebastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);

struct t_weechat_plugin *weechat_xfer_plugin = NULL;

char *xfer_type_string[] =                 /* strings for types             */
{ "file_recv", "file_send", "chat_recv",
  "chat_send"
};

char *xfer_protocol_string[] =             /* strings for protocols         */
{ "none", "dcc"
};

char *xfer_status_string[] =               /* strings for status            */
{ N_("waiting"), N_("connecting"),
  N_("active"), N_("done"), N_("failed"),
  N_("aborted")
};

struct t_xfer *xfer_list = NULL;       /* list of files/chats               */
struct t_xfer *last_xfer = NULL;       /* last file/chat in list            */
int xfer_count = 0;                    /* number of xfer                    */

int xfer_signal_upgrade_received = 0;  /* signal "upgrade" received ?       */


/*
 * xfer_valid: check if a xfer pointer exists
 *             return 1 if xfer exists
 *                    0 if xfer is not found
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
 * xfer_signal_upgrade_cb: callback for "upgrade" signal
 */

int
xfer_signal_upgrade_cb (void *data, const char *signal, const char *type_data,
                        void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    xfer_signal_upgrade_received = 1;

    return WEECHAT_RC_OK;
}


/*
 * xfer_create_directories: create directories for xfer plugin
 */

void
xfer_create_directories ()
{
    const char *weechat_dir;
    char *dir1, *dir2;

    /* create download directory */
    weechat_dir = weechat_info_get ("weechat_dir", "");
    if (weechat_dir)
    {
        dir1 = weechat_string_expand_home (weechat_config_string (xfer_config_file_download_path));
        dir2 = weechat_string_replace (dir1, "%h", weechat_dir);
        if (dir2)
            (void) weechat_mkdir (dir2, 0700);
        if (dir1)
            free (dir1);
        if (dir2)
            free (dir2);
    }
}

/*
 * xfer_search_type: search xfer type with a string
 *                   return -1 if not found
 */

int
xfer_search_type (const char *type)
{
    int i;

    for (i = 0; i < XFER_NUM_TYPES; i++)
    {
        if (weechat_strcasecmp (xfer_type_string[i], type) == 0)
            return i;
    }

    /* xfer type not found */
    return -1;
}

/*
 * xfer_search_protocol: search xfer protocol with a string
 *                       return -1 if not found
 */

int
xfer_search_protocol (const char *protocol)
{
    int i;

    for (i = 0; i < XFER_NUM_PROTOCOLS; i++)
    {
        if (weechat_strcasecmp (xfer_protocol_string[i], protocol) == 0)
            return i;
    }

    /* xfer protocol not found */
    return -1;
}

/*
 * xfer_search: search a xfer
 */

struct t_xfer *
xfer_search (const char *plugin_name, const char *plugin_id, enum t_xfer_type type,
             enum t_xfer_status status, int port)
{
    struct t_xfer *ptr_xfer;

    for (ptr_xfer = xfer_list; ptr_xfer; ptr_xfer = ptr_xfer->next_xfer)
    {
        if ((weechat_strcasecmp (ptr_xfer->plugin_name, plugin_name) == 0)
            && (weechat_strcasecmp (ptr_xfer->plugin_id, plugin_id) == 0)
            && (ptr_xfer->type == type)
            && (ptr_xfer->status = status)
            && (ptr_xfer->port == port))
            return ptr_xfer;
    }

    /* xfer not found */
    return NULL;
}

/*
 * xfer_search_by_number: search a xfer by number (first xfer is 0)
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
 * xfer_search_by_buffer: search a xfer by buffer (for chat only)
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
 * xfer_close: close a xfer
 */

void
xfer_close (struct t_xfer *xfer, enum t_xfer_status status)
{
    struct stat st;

    xfer->status = status;

    if (XFER_HAS_ENDED(xfer->status))
    {
        xfer_send_signal (xfer, "xfer_ended");

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
        if (XFER_IS_FILE(xfer->type))
        {
            weechat_printf (NULL,
                            _("%s%s: file %s %s %s: %s"),
                            (xfer->status == XFER_STATUS_DONE) ?
                            "" : weechat_prefix ("error"),
                            XFER_PLUGIN_NAME,
                            xfer->filename,
                            (xfer->type == XFER_TYPE_FILE_SEND) ?
                            _("sent to") : _("received from"),
                            xfer->remote_nick,
                            (xfer->status == XFER_STATUS_DONE) ?
                            _("OK") : _("FAILED"));
            xfer_network_child_kill (xfer);
        }
    }
    if (xfer->status == XFER_STATUS_ABORTED)
    {
        if (XFER_IS_CHAT(xfer->type))
        {
            weechat_printf (xfer->buffer,
                            _("%s: chat closed with %s "
                              "(%d.%d.%d.%d)"),
                            XFER_PLUGIN_NAME,
                            xfer->remote_nick,
                            xfer->address >> 24,
                            (xfer->address >> 16) & 0xff,
                            (xfer->address >> 8) & 0xff,
                            xfer->address & 0xff);
        }
    }

    /* remove empty file if received file failed and nothing was transfered */
    if (((xfer->status == XFER_STATUS_FAILED)
         || (xfer->status == XFER_STATUS_ABORTED))
        && XFER_IS_FILE(xfer->type)
        && XFER_IS_RECV(xfer->type)
        && xfer->local_filename
        && xfer->pos == 0)
    {
        /* erase file only if really empty on disk */
        if (stat (xfer->local_filename, &st) != -1)
        {
            if ((unsigned long long) st.st_size == 0)
                unlink (xfer->local_filename);
        }
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
}

/*
 * xfer_port_in_use: return 1 if a port is in used
 *                   (by an active or connecting xfer)
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
 * xfer_send_signal: send a signal for a xfer
 */

void
xfer_send_signal (struct t_xfer *xfer, const char *signal)
{
    struct t_infolist *infolist;
    struct t_infolist_item *item;
    char str_long[128];

    infolist = weechat_infolist_new ();
    if (infolist)
    {
        item = weechat_infolist_new_item (infolist);
        if (item)
        {
            weechat_infolist_new_var_string (item, "plugin_name",
                                             xfer->plugin_name);
            weechat_infolist_new_var_string (item, "plugin_id",
                                             xfer->plugin_id);
            weechat_infolist_new_var_string (item, "type",
                                             xfer_type_string[xfer->type]);
            weechat_infolist_new_var_string (item, "protocol",
                                             xfer_protocol_string[xfer->protocol]);
            weechat_infolist_new_var_string (item, "remote_nick",
                                             xfer->remote_nick);
            weechat_infolist_new_var_string (item, "local_nick",
                                             xfer->local_nick);
            weechat_infolist_new_var_string (item, "charset_modifier",
                                             xfer->charset_modifier);
            weechat_infolist_new_var_string (item, "filename",
                                             xfer->filename);
            snprintf (str_long, sizeof (str_long), "%llu", xfer->size);
            weechat_infolist_new_var_string (item, "size",
                                             str_long);
            snprintf (str_long, sizeof (str_long), "%llu", xfer->start_resume);
            weechat_infolist_new_var_string (item, "start_resume",
                                             str_long);
            snprintf (str_long, sizeof (str_long), "%lu", xfer->address);
            weechat_infolist_new_var_string (item, "address",
                                             str_long);
            weechat_infolist_new_var_integer (item, "port",
                                              xfer->port);

            weechat_hook_signal_send (signal, WEECHAT_HOOK_SIGNAL_POINTER,
                                      infolist);
        }
        weechat_infolist_free (infolist);
    }
}

/*
 * xfer_alloc: allocate a new xfer
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

    /* default values */
    new_xfer->filename = NULL;
    new_xfer->size = 0;
    new_xfer->address = 0;
    new_xfer->port = 0;
    new_xfer->remote_nick = NULL;
    new_xfer->local_nick = NULL;
    new_xfer->charset_modifier = NULL;

    new_xfer->type = 0;
    new_xfer->protocol = 0;
    new_xfer->status = 0;
    new_xfer->buffer = NULL;
    new_xfer->remote_nick_color = NULL;
    new_xfer->fast_send = weechat_config_boolean (xfer_config_network_fast_send);
    new_xfer->blocksize = weechat_config_integer (xfer_config_network_blocksize);
    new_xfer->start_time = time_now;
    new_xfer->start_transfer = time_now;
    new_xfer->sock = -1;
    new_xfer->child_pid = 0;
    new_xfer->child_read = -1;
    new_xfer->child_write = -1;
    new_xfer->hook_fd = NULL;
    new_xfer->hook_timer = NULL;
    new_xfer->unterminated_message = NULL;
    new_xfer->file = -1;
    new_xfer->local_filename = NULL;
    new_xfer->filename_suffix = -1;
    new_xfer->pos = 0;
    new_xfer->ack = 0;
    new_xfer->start_resume = 0;
    new_xfer->last_check_time = time_now;
    new_xfer->last_check_pos = time_now;
    new_xfer->last_activity = 0;
    new_xfer->bytes_per_sec = 0;
    new_xfer->eta = 0;

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
 * xfer_new: add a xfer to list
 */

struct t_xfer *
xfer_new (const char *plugin_name, const char *plugin_id,
          enum t_xfer_type type, enum t_xfer_protocol protocol,
          const char *remote_nick, const char *local_nick,
          const char *charset_modifier, const char *filename,
          unsigned long long size, const char *proxy, unsigned long address,
          int port, int sock, const char *local_filename)
{
    struct t_xfer *new_xfer;
    const char *ptr_color;

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
    ptr_color = weechat_info_get ("irc_nick_color", remote_nick);
    new_xfer->remote_nick_color = (ptr_color) ? strdup (ptr_color) : NULL;
    new_xfer->local_nick = (local_nick) ? strdup (local_nick) : NULL;
    new_xfer->charset_modifier = (charset_modifier) ? strdup (charset_modifier) : NULL;
    if (XFER_IS_FILE(type))
        new_xfer->filename = (filename) ? strdup (filename) : NULL;
    else
        new_xfer->filename = strdup (_("xfer chat"));
    new_xfer->size = size;
    new_xfer->proxy = (proxy) ? strdup (proxy) : NULL;
    new_xfer->address = address;
    new_xfer->port = port;

    new_xfer->status = XFER_STATUS_WAITING;
    new_xfer->sock = sock;
    if (local_filename)
        new_xfer->local_filename = strdup (local_filename);
    else
        xfer_file_find_filename (new_xfer);

    /* write info message on core buffer */
    switch (type)
    {
        case XFER_TYPE_FILE_RECV:
            weechat_printf (NULL,
                            _("%s: incoming file from %s "
                              "(%s.%s), ip: %d.%d.%d.%d, name: %s, %llu bytes "
                              "(protocol: %s)"),
                            XFER_PLUGIN_NAME,
                            remote_nick,
                            plugin_name,
                            plugin_id,
                            address >> 24,
                            (address >> 16) & 0xff,
                            (address >> 8) & 0xff,
                            address & 0xff,
                            filename,
                            size,
                            xfer_protocol_string[protocol]);
            xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
            break;
        case XFER_TYPE_FILE_SEND:
            weechat_printf (NULL,
                            _("%s: sending file to %s (%s.%s): %s "
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
                              "(%s.%s), ip: %d.%d.%d.%d"),
                            XFER_PLUGIN_NAME,
                            remote_nick,
                            plugin_name,
                            plugin_id,
                            address >> 24,
                            (address >> 16) & 0xff,
                            (address >> 8) & 0xff,
                            address & 0xff);
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

    if ( ( (type == XFER_TYPE_FILE_RECV)
                && (weechat_config_boolean (xfer_config_file_auto_accept_files)) )
         || ( (type == XFER_TYPE_CHAT_RECV)
              && (weechat_config_boolean (xfer_config_file_auto_accept_chats)) ) )
        xfer_network_accept (new_xfer);
    else
        xfer_buffer_refresh (WEECHAT_HOTLIST_PRIVATE);

    return new_xfer;
}

/*
 * xfer_free: free xfer struct and remove it from list
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
    if (xfer->plugin_id)
        free (xfer->plugin_id);
    if (xfer->remote_nick)
        free (xfer->remote_nick);
    if (xfer->local_nick)
        free (xfer->local_nick);
    if (xfer->charset_modifier)
        free (xfer->charset_modifier);
    if (xfer->filename)
        free (xfer->filename);
    if (xfer->remote_nick_color)
        free (xfer->remote_nick_color);
    if (xfer->unterminated_message)
        free (xfer->unterminated_message);
    if (xfer->local_filename)
        free (xfer->local_filename);

    free (xfer);

    xfer_list = new_xfer_list;

    xfer_count--;
    if (xfer_buffer_selected_line >= xfer_count)
        xfer_buffer_selected_line = (xfer_count == 0) ? 0 : xfer_count - 1;
}

/*
 * xfer_add_cb: callback for "xfer_add" signal
 */

int
xfer_add_cb (void *data, const char *signal, const char *type_data,
             void *signal_data)
{
    struct t_infolist *infolist;
    const char *plugin_name, *plugin_id, *str_type, *str_protocol;
    const char *remote_nick, *local_nick, *charset_modifier, *filename, *proxy;
    int type, protocol, args, port_start, port_end, sock, port;
    const char *weechat_dir;
    char *dir1, *dir2, *filename2, *short_filename, *pos;
    struct stat st;
    struct hostent *host;
    struct sockaddr_in addr;
    socklen_t length;
    struct in_addr tmpaddr;
    unsigned long local_addr;
    unsigned long long file_size;
    struct t_xfer *ptr_xfer;

    /* make C compiler happy */
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

    sock = -1;
    port = 0;

    plugin_name = weechat_infolist_string (infolist, "plugin_name");
    plugin_id = weechat_infolist_string (infolist, "plugin_id");
    str_type = weechat_infolist_string (infolist, "type");
    str_protocol = weechat_infolist_string (infolist, "protocol");
    remote_nick = weechat_infolist_string (infolist, "remote_nick");
    local_nick = weechat_infolist_string (infolist, "local_nick");
    charset_modifier = weechat_infolist_string (infolist, "charset_modifier");
    filename = weechat_infolist_string (infolist, "filename");
    proxy = weechat_infolist_string (infolist, "proxy");
    protocol = XFER_NO_PROTOCOL;

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

    filename2 = NULL;
    file_size = 0;
    port = 0;

    if (type == XFER_TYPE_FILE_RECV)
    {
        filename2 = strdup (filename);
        sscanf (weechat_infolist_string (infolist, "size"), "%llu", &file_size);
    }

    if (type == XFER_TYPE_FILE_SEND)
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
            dir1 = weechat_string_expand_home (weechat_config_string (xfer_config_file_upload_path));
            if (!dir1)
            {
                weechat_printf (NULL,
                                _("%s%s: not enough memory"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME);
                goto error;
            }

            weechat_dir = weechat_info_get ("weechat_dir", "");
            dir2 = weechat_string_replace (dir1, "%h", weechat_dir);
            if (!dir2)
            {
                weechat_printf (NULL,
                                _("%s%s: not enough memory"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME);
                free (dir1);
                goto error;
            }
            filename2 = malloc (strlen (dir2) + strlen (filename) + 4);
            if (!filename2)
            {
                weechat_printf (NULL,
                                _("%s%s: not enough memory"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME);
                free (dir1);
                free (dir2);
                goto error;
            }
            strcpy (filename2, dir2);
            if (filename2[strlen (filename2) - 1] != DIR_SEPARATOR_CHAR)
                strcat (filename2, DIR_SEPARATOR);
            strcat (filename2, filename);
            if (dir1)
                free (dir1);
            if (dir2)
                free (dir2);
        }
#endif
        /* check if file exists */
        if (stat (filename2, &st) == -1)
        {
            weechat_printf (NULL,
                            _("%s%s: cannot access file \"%s\""),
                            weechat_prefix ("error"), XFER_PLUGIN_NAME,
                            filename2);
            if (filename2)
                free (filename2);
            goto error;
        }
        file_size = st.st_size;
    }

    if (XFER_IS_RECV(type))
    {
        sscanf (weechat_infolist_string (infolist, "address"), "%lu", &local_addr);
        port = weechat_infolist_integer (infolist, "port");
    }
    else
    {
        /* get local IP address */
        sscanf (weechat_infolist_string (infolist, "address"), "%lu", &local_addr);

        memset (&addr, 0, sizeof (struct sockaddr_in));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl (local_addr);

        /* look up the IP address from network_own_ip, if set */
        if (weechat_config_string(xfer_config_network_own_ip)
            && weechat_config_string(xfer_config_network_own_ip)[0])
        {
            host = gethostbyname (weechat_config_string (xfer_config_network_own_ip));
            if (host)
            {
                memcpy (&tmpaddr, host->h_addr_list[0], sizeof(struct in_addr));
                local_addr = ntohl (tmpaddr.s_addr);

                sock = weechat_infolist_integer (infolist, "socket");
                if (sock > 0)
                {
                    memset (&addr, 0, sizeof (struct sockaddr_in));
                    length = sizeof (addr);
                    getsockname (sock, (struct sockaddr *) &addr, &length);
                    addr.sin_family = AF_INET;
                }
            }
            else
            {
                weechat_printf (NULL,
                                _("%s%s: could not find address for \"%s\", "
                                  "falling back to local IP"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME,
                                weechat_config_string (xfer_config_network_own_ip));
            }
        }

        /* open socket for xfer */
        sock = socket (AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
        {
            weechat_printf (NULL,
                            _("%s%s: cannot create socket for xfer"),
                            weechat_prefix ("error"), XFER_PLUGIN_NAME);
            if (filename2)
                free (filename2);
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
                        addr.sin_port = htons (port);
                        if (bind (sock, (struct sockaddr *) &addr, sizeof (addr)) == 0)
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
            addr.sin_port = 0;
            if (bind (sock, (struct sockaddr *) &addr, sizeof (addr)) == 0)
            {
                length = sizeof (addr);
                getsockname (sock, (struct sockaddr *) &addr, &length);
                port = ntohs (addr.sin_port);
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
            if (filename2)
                free (filename2);
            goto error;
        }
    }

    if (XFER_IS_FILE(type))
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

    if (type == XFER_TYPE_FILE_RECV)
    {
        if (filename2)
        {
            free (filename2);
            filename2 = NULL;
        }
    }

    /* add xfer entry and listen to socket if type is file or chat "send" */
    if (XFER_IS_FILE(type))
        ptr_xfer = xfer_new (plugin_name, plugin_id, type, protocol,
                             remote_nick, local_nick, charset_modifier,
                             short_filename, file_size, proxy, local_addr,
                             port, sock, filename2);
    else
        ptr_xfer = xfer_new (plugin_name, plugin_id, type, protocol,
                             remote_nick, local_nick, charset_modifier, NULL,
                             0, proxy, local_addr, port, sock, NULL);

    if (!ptr_xfer)
    {
        weechat_printf (NULL,
                        _("%s%s: error creating xfer"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME);
        close (sock);
        if (short_filename)
            free (short_filename);
        if (filename2)
            free (filename2);
        goto error;
    }

    /* send signal if type is file or chat "send" */
    if (XFER_IS_SEND(ptr_xfer->type) && !XFER_HAS_ENDED(ptr_xfer->status))
        xfer_send_signal (ptr_xfer, "xfer_send_ready");

    if (short_filename)
        free (short_filename);
    if (filename2)
        free (filename2);

    weechat_infolist_reset_item_cursor (infolist);
    return WEECHAT_RC_OK;

error:
    weechat_infolist_reset_item_cursor (infolist);
    return WEECHAT_RC_ERROR;
}

/*
 * xfer_start_resume_cb: callback called when resume is accepted by sender
 */

int
xfer_start_resume_cb (void *data, const char *signal, const char *type_data,
                      void *signal_data)
{
    struct t_infolist *infolist;
    struct t_xfer *ptr_xfer;
    const char *plugin_name, *plugin_id, *filename, *str_start_resume;
    int port;
    unsigned long long start_resume;

    /* make C compiler happy */
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

    if (!plugin_name || !plugin_id || !filename || !str_start_resume)
    {
        weechat_printf (NULL,
                        _("%s%s: missing arguments (%s)"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME,
                        "xfer_start_resume");
        goto error;
    }

    sscanf (str_start_resume, "%llu", &start_resume);

    ptr_xfer = xfer_search (plugin_name, plugin_id, XFER_TYPE_FILE_RECV,
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
 * xfer_accept_resume_cb: callback called when sender receives resume request
 *                        from recever
 */

int
xfer_accept_resume_cb (void *data, const char *signal, const char *type_data,
                       void *signal_data)
{
    struct t_infolist *infolist;
    struct t_xfer *ptr_xfer;
    const char *plugin_name, *plugin_id, *filename, *str_start_resume;
    int port;
    unsigned long long start_resume;

    /* make C compiler happy */
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

    ptr_xfer = xfer_search (plugin_name, plugin_id, XFER_TYPE_FILE_SEND,
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
 * xfer_add_to_infolist: add a xfer in an infolist
 *                       return 1 if ok, 0 if error
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
    snprintf (value, sizeof (value), "%lu", xfer->address);
    if (!weechat_infolist_new_var_string (ptr_item, "address", value))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "port", xfer->port))
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
    if (!weechat_infolist_new_var_string (ptr_item, "unterminated_message", xfer->unterminated_message))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "file", xfer->file))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "local_filename", xfer->local_filename))
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

    return 1;
}

/*
 * xfer_print_log: print xfer infos in log (usually for crash dump)
 */

void
xfer_print_log ()
{
    struct t_xfer *ptr_xfer;

    for (ptr_xfer = xfer_list; ptr_xfer; ptr_xfer = ptr_xfer->next_xfer)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[xfer (addr:0x%lx)]", ptr_xfer);
        weechat_log_printf ("  plugin_name . . . . : '%s'",  ptr_xfer->plugin_name);
        weechat_log_printf ("  plugin_id . . . . . : '%s'",  ptr_xfer->plugin_id);
        weechat_log_printf ("  type. . . . . . . . : %d (%s)",
                            ptr_xfer->type,
                            xfer_type_string[ptr_xfer->type]);
        weechat_log_printf ("  protocol. . . . . . : %d (%s)",
                            ptr_xfer->protocol,
                            xfer_protocol_string[ptr_xfer->protocol]);
        weechat_log_printf ("  remote_nick . . . . : '%s'",  ptr_xfer->remote_nick);
        weechat_log_printf ("  local_nick. . . . . : '%s'",  ptr_xfer->local_nick);
        weechat_log_printf ("  charset_modifier. . : '%s'",  ptr_xfer->charset_modifier);
        weechat_log_printf ("  filename. . . . . . : '%s'",  ptr_xfer->filename);
        weechat_log_printf ("  size. . . . . . . . : %llu",  ptr_xfer->size);
        weechat_log_printf ("  proxy . . . . . . . : '%s'",  ptr_xfer->proxy);
        weechat_log_printf ("  address . . . . . . : %lu",   ptr_xfer->address);
        weechat_log_printf ("  port. . . . . . . . : %d",    ptr_xfer->port);

        weechat_log_printf ("  status. . . . . . . : %d (%s)",
                            ptr_xfer->status,
                            xfer_status_string[ptr_xfer->status]);
        weechat_log_printf ("  buffer. . . . . . . : 0x%lx", ptr_xfer->buffer);
        weechat_log_printf ("  remote_nick_color . : '%s'",  ptr_xfer->remote_nick_color);
        weechat_log_printf ("  fast_send . . . . . : %d",    ptr_xfer->fast_send);
        weechat_log_printf ("  blocksize . . . . . : %d",    ptr_xfer->blocksize);
        weechat_log_printf ("  start_time. . . . . : %ld",   ptr_xfer->start_time);
        weechat_log_printf ("  start_transfer. . . : %ld",   ptr_xfer->start_transfer);
        weechat_log_printf ("  sock. . . . . . . . : %d",    ptr_xfer->sock);
        weechat_log_printf ("  child_pid . . . . . : %d",    ptr_xfer->child_pid);
        weechat_log_printf ("  child_read. . . . . : %d",    ptr_xfer->child_read);
        weechat_log_printf ("  child_write . . . . : %d",    ptr_xfer->child_write);
        weechat_log_printf ("  hook_fd . . . . . . : 0x%lx", ptr_xfer->hook_fd);
        weechat_log_printf ("  hook_timer. . . . . : 0x%lx", ptr_xfer->hook_timer);
        weechat_log_printf ("  unterminated_message: '%s'",  ptr_xfer->unterminated_message);
        weechat_log_printf ("  file. . . . . . . . : %d",    ptr_xfer->file);
        weechat_log_printf ("  local_filename. . . : '%s'",  ptr_xfer->local_filename);
        weechat_log_printf ("  filename_suffix . . : %d",    ptr_xfer->filename_suffix);
        weechat_log_printf ("  pos . . . . . . . . : %llu",  ptr_xfer->pos);
        weechat_log_printf ("  ack . . . . . . . . : %llu",  ptr_xfer->ack);
        weechat_log_printf ("  start_resume. . . . : %llu",  ptr_xfer->start_resume);
        weechat_log_printf ("  last_check_time . . : %ld",   ptr_xfer->last_check_time);
        weechat_log_printf ("  last_check_pos. . . : %llu",  ptr_xfer->last_check_pos);
        weechat_log_printf ("  last_activity . . . : %ld",   ptr_xfer->last_activity);
        weechat_log_printf ("  bytes_per_sec . . . : %llu",  ptr_xfer->bytes_per_sec);
        weechat_log_printf ("  eta . . . . . . . . : %llu",  ptr_xfer->eta);
        weechat_log_printf ("  prev_xfer . . . . . : 0x%lx", ptr_xfer->prev_xfer);
        weechat_log_printf ("  next_xfer . . . . . : 0x%lx", ptr_xfer->next_xfer);
    }
}

/*
 * xfer_debug_dump_cb: callback for "debug_dump" signal
 */

int
xfer_debug_dump_cb (void *data, const char *signal, const char *type_data,
                    void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data
        || (weechat_strcasecmp ((char *)signal_data, XFER_PLUGIN_NAME) == 0))
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
 * weechat_plugin_init: initialize xfer plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    int i, upgrading;

    weechat_plugin = plugin;

    if (!xfer_config_init ())
        return WEECHAT_RC_ERROR;

    if (xfer_config_read () < 0)
        return WEECHAT_RC_ERROR;

    xfer_create_directories ();

    xfer_command_init ();

    /* hook some signals */
    weechat_hook_signal ("upgrade", &xfer_signal_upgrade_cb, NULL);
    weechat_hook_signal ("xfer_add", &xfer_add_cb, NULL);
    weechat_hook_signal ("xfer_start_resume", &xfer_start_resume_cb, NULL);
    weechat_hook_signal ("xfer_accept_resume", &xfer_accept_resume_cb, NULL);
    weechat_hook_signal ("debug_dump", &xfer_debug_dump_cb, NULL);

    /* hook completions */
    xfer_completion_init ();

    xfer_info_init ();

    /* look at arguments */
    upgrading = 0;
    for (i = 0; i < argc; i++)
    {
        if (weechat_strcasecmp (argv[i], "--upgrade") == 0)
            upgrading = 1;
    }

    if (upgrading)
        xfer_upgrade_load ();

    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: end xfer plugin
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    struct t_xfer *ptr_xfer;

    /* make C compiler happy */
    (void) plugin;

    xfer_config_write ();

    if (xfer_signal_upgrade_received)
        xfer_upgrade_save ();
    else
    {
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
                xfer_close (ptr_xfer, XFER_STATUS_FAILED);
            }
        }
    }

    return WEECHAT_RC_OK;
}
