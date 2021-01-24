/*
 * xfer-config.c - xfer configuration options (file xfer.conf)
 *
 * Copyright (C) 2003-2021 Sébastien Helleu <flashcode@flashtux.org>
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
#include <limits.h>

#include "../weechat-plugin.h"
#include "xfer.h"
#include "xfer-config.h"
#include "xfer-buffer.h"


struct t_config_file *xfer_config_file = NULL;

/* xfer config, look section */

struct t_config_option *xfer_config_look_auto_open_buffer;
struct t_config_option *xfer_config_look_progress_bar_size;
struct t_config_option *xfer_config_look_pv_tags;

/* xfer config, color section */

struct t_config_option *xfer_config_color_status[XFER_NUM_STATUS];
struct t_config_option *xfer_config_color_text;
struct t_config_option *xfer_config_color_text_bg;
struct t_config_option *xfer_config_color_text_selected;

/* xfer config, network section */

struct t_config_option *xfer_config_network_blocksize;
struct t_config_option *xfer_config_network_fast_send;
struct t_config_option *xfer_config_network_own_ip;
struct t_config_option *xfer_config_network_port_range;
struct t_config_option *xfer_config_network_send_ack;
struct t_config_option *xfer_config_network_speed_limit_recv;
struct t_config_option *xfer_config_network_speed_limit_send;
struct t_config_option *xfer_config_network_timeout;

/* xfer config, file section */

struct t_config_option *xfer_config_file_auto_accept_chats;
struct t_config_option *xfer_config_file_auto_accept_files;
struct t_config_option *xfer_config_file_auto_accept_nicks;
struct t_config_option *xfer_config_file_auto_check_crc32;
struct t_config_option *xfer_config_file_auto_rename;
struct t_config_option *xfer_config_file_auto_resume;
struct t_config_option *xfer_config_file_convert_spaces;
struct t_config_option *xfer_config_file_download_path;
struct t_config_option *xfer_config_file_download_temporary_suffix;
struct t_config_option *xfer_config_file_upload_path;
struct t_config_option *xfer_config_file_use_nick_in_filename;


/*
 * Callback for changes on an option that requires a refresh of xfer list.
 */

void
xfer_config_refresh_cb (const void *pointer, void *data,
                        struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (xfer_buffer)
        xfer_buffer_refresh (NULL);
}

/*
 * Reloads xfer configuration file.
 */

int
xfer_config_reload (const void *pointer, void *data,
                    struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    return  weechat_config_reload (config_file);
}

/*
 * Initializes xfer configuration file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
xfer_config_init ()
{
    struct t_config_section *ptr_section;

    xfer_config_file = weechat_config_new (XFER_CONFIG_NAME,
                                           &xfer_config_reload, NULL, NULL);
    if (!xfer_config_file)
        return 0;

    ptr_section = weechat_config_new_section (xfer_config_file, "look",
                                              0, 0,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (xfer_config_file);
        xfer_config_file = NULL;
        return 0;
    }

    xfer_config_look_auto_open_buffer = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "auto_open_buffer", "boolean",
        N_("auto open xfer buffer when a new xfer is added "
           "to list"),
        NULL, 0, 0, "on", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    xfer_config_look_progress_bar_size = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "progress_bar_size", "integer",
        N_("size of progress bar, in chars (if 0, progress bar is disabled)"),
        NULL, 0, XFER_CONFIG_PROGRESS_BAR_MAX_SIZE, "20", NULL, 0,
        NULL, NULL, NULL,
        &xfer_config_refresh_cb, NULL, NULL,
        NULL, NULL, NULL);
    xfer_config_look_pv_tags = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "pv_tags", "string",
        N_("comma separated list of tags used in private messages, for example: "
           "\"notify_message\", \"notify_private\" or \"notify_highlight\""),
        NULL, 0, 0, "notify_private", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    ptr_section = weechat_config_new_section (xfer_config_file, "color",
                                              0, 0,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (xfer_config_file);
        xfer_config_file = NULL;
        return 0;
    }

    xfer_config_color_status[XFER_STATUS_ABORTED] = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "status_aborted", "color",
        N_("text color for \"aborted\" status"),
        NULL, 0, 0, "lightred", NULL, 0,
        NULL, NULL, NULL,
        &xfer_config_refresh_cb, NULL, NULL,
        NULL, NULL, NULL);
    xfer_config_color_status[XFER_STATUS_ACTIVE] = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "status_active", "color",
        N_("text color for \"active\" status"),
        NULL, 0, 0, "lightblue", NULL, 0,
        NULL, NULL, NULL,
        &xfer_config_refresh_cb, NULL, NULL,
        NULL, NULL, NULL);
    xfer_config_color_status[XFER_STATUS_CONNECTING] = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "status_connecting", "color",
        N_("text color for \"connecting\" status"),
        NULL, 0, 0, "yellow", NULL, 0,
        NULL, NULL, NULL,
        &xfer_config_refresh_cb, NULL, NULL,
        NULL, NULL, NULL);
    xfer_config_color_status[XFER_STATUS_DONE] = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "status_done", "color",
        N_("text color for \"done\" status"),
        NULL, 0, 0, "lightgreen", NULL, 0,
        NULL, NULL, NULL,
        &xfer_config_refresh_cb, NULL, NULL,
        NULL, NULL, NULL);
    xfer_config_color_status[XFER_STATUS_FAILED] = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "status_failed", "color",
        N_("text color for \"failed\" status"),
        NULL, 0, 0, "lightred", NULL, 0,
        NULL, NULL, NULL,
        &xfer_config_refresh_cb, NULL, NULL,
        NULL, NULL, NULL);
    xfer_config_color_status[XFER_STATUS_WAITING] = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "status_waiting", "color",
        N_("text color for \"waiting\" status"),
        NULL, 0, 0, "lightcyan", NULL, 0,
        NULL, NULL, NULL,
        &xfer_config_refresh_cb, NULL, NULL,
        NULL, NULL, NULL);
    xfer_config_color_text = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "text", "color",
        N_("text color in xfer buffer"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &xfer_config_refresh_cb, NULL, NULL,
        NULL, NULL, NULL);
    xfer_config_color_text_bg = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "text_bg", "color",
        N_("background color in xfer buffer"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &xfer_config_refresh_cb, NULL, NULL,
        NULL, NULL, NULL);
    xfer_config_color_text_selected = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "text_selected", "color",
        N_("text color of selected line in xfer buffer"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &xfer_config_refresh_cb, NULL, NULL,
        NULL, NULL, NULL);

    ptr_section = weechat_config_new_section (xfer_config_file, "network",
                                              0, 0,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (xfer_config_file);
        xfer_config_file = NULL;
        return 0;
    }

    xfer_config_network_blocksize = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "blocksize", "integer",
        N_("block size for sending packets, in bytes"),
        NULL, XFER_BLOCKSIZE_MIN, XFER_BLOCKSIZE_MAX, "65536", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    xfer_config_network_fast_send = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "fast_send", "boolean",
        N_("does not wait for ACK when sending file"),
        NULL, 0, 0, "on", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    xfer_config_network_own_ip = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "own_ip", "string",
        N_("IP or DNS address used for sending files/chats "
           "(if empty, local interface IP is used)"),
        NULL, 0, 0, "", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    xfer_config_network_port_range = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "port_range", "string",
        N_("restricts outgoing files/chats to use only ports in the given "
           "range (useful for NAT) (syntax: a single port, ie. 5000 or a port "
           "range, ie. 5000-5015, empty value means any port, it's recommended "
           "to use ports greater than 1024, because only root can use ports "
           "below 1024)"),
        NULL, 0, 0, "", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    xfer_config_network_send_ack = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "send_ack", "boolean",
        N_("send acks when receiving files; if disabled, the transfer may "
           "freeze if the sender is waiting for acks (for example a WeeChat "
           "sending a file with option xfer.network.fast_send set to off); "
           "on the other hand, disabling send of acks may prevent a freeze if "
           "the acks are not sent immediately to the sender"),
        NULL, 0, 0, "on", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    xfer_config_network_speed_limit_recv = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "speed_limit_recv", "integer",
        N_("speed limit for receiving files, in kilo-bytes by second (0 means "
           "no limit)"),
        NULL, 0, INT_MAX, "0", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    xfer_config_network_speed_limit_send = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "speed_limit_send", "integer",
        N_("speed limit for sending files, in kilo-bytes by second (0 means "
           "no limit)"),
        NULL, 0, INT_MAX, "0", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    xfer_config_network_timeout = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "timeout", "integer",
        N_("timeout for xfer request (in seconds)"),
        NULL, 5, INT_MAX, "300", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    ptr_section = weechat_config_new_section (xfer_config_file, "file",
                                              0, 0,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (xfer_config_file);
        xfer_config_file = NULL;
        return 0;
    }

    xfer_config_file_auto_accept_chats = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "auto_accept_chats", "boolean",
        N_("automatically accept chat requests (use carefully!)"),
        NULL, 0, 0, "off", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    xfer_config_file_auto_accept_files = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "auto_accept_files", "boolean",
        N_("automatically accept incoming files (use carefully!)"),
        NULL, 0, 0, "off", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    xfer_config_file_auto_accept_nicks = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "auto_accept_nicks", "string",
        N_("comma-separated list of nicks for which the incoming files and "
           "chats are automatically accepted; format is \"server.nick\" (for a "
           "specific server) or \"nick\" (for all servers); example: "
           "\"freenode.FlashCode,andrew\""),
        NULL, 0, 0, "", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    xfer_config_file_auto_check_crc32 = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "auto_check_crc32", "boolean",
        N_("automatically check CRC32 file checksum if it is found in the "
           "filename (8 hexadecimal chars)"),
        NULL, 0, 0, "off", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    xfer_config_file_auto_rename = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "auto_rename", "boolean",
        N_("rename incoming files if already exists (add \".1\", \".2\", ...)"),
        NULL, 0, 0, "on", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    xfer_config_file_auto_resume = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "auto_resume", "boolean",
        N_("automatically resume file transfer if connection with remote host "
           "is lost"),
        NULL, 0, 0, "on", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    xfer_config_file_convert_spaces = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "convert_spaces", "boolean",
        N_("convert spaces to underscores when sending and receiving files"),
        NULL, 0, 0, "on", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    xfer_config_file_download_path = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "download_path", "string",
        N_("path for writing incoming files: \"%h\" at beginning of string is "
           "replaced by WeeChat home (\"~/.weechat\" by default) "
           "(note: content is evaluated, see /help eval)"),
        NULL, 0, 0, "%h/xfer", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    xfer_config_file_download_temporary_suffix = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "download_temporary_suffix", "string",
        N_("temporary filename suffix used during the transfer for a file "
           "received, it is removed after successful transfer; "
           "if empty string, no filename suffix is used during the transfer"),
        NULL, 0, 0, ".part", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    xfer_config_file_upload_path = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "upload_path", "string",
        N_("path for reading files when sending (when no path is specified "
           "by user): \"%h\" at beginning of string is replaced by WeeChat "
           "home (\"~/.weechat\" by default) "
           "(note: content is evaluated, see /help eval)"),
        NULL, 0, 0, "~", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    xfer_config_file_use_nick_in_filename = weechat_config_new_option (
        xfer_config_file, ptr_section,
        "use_nick_in_filename", "boolean",
        N_("use remote nick as prefix in local filename when receiving a file"),
        NULL, 0, 0, "on", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    return 1;
}

/*
 * Reads xfer configuration file.
 */

int
xfer_config_read ()
{
    return weechat_config_read (xfer_config_file);
}

/*
 * Writes xfer configuration file.
 */

int
xfer_config_write ()
{
    return weechat_config_write (xfer_config_file);
}
