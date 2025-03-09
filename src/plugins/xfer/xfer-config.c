/*
 * xfer-config.c - xfer configuration options (file xfer.conf)
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
#include <limits.h>

#include "../weechat-plugin.h"
#include "xfer.h"
#include "xfer-config.h"
#include "xfer-buffer.h"


struct t_config_file *xfer_config_file = NULL;

/* sections */

struct t_config_section *xfer_config_section_look = NULL;
struct t_config_section *xfer_config_section_color = NULL;
struct t_config_section *xfer_config_section_network = NULL;
struct t_config_section *xfer_config_section_file = NULL;

/* xfer config, look section */

struct t_config_option *xfer_config_look_auto_open_buffer = NULL;
struct t_config_option *xfer_config_look_progress_bar_size = NULL;
struct t_config_option *xfer_config_look_pv_tags = NULL;

/* xfer config, color section */

struct t_config_option *xfer_config_color_status[XFER_NUM_STATUS] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};
struct t_config_option *xfer_config_color_text = NULL;
struct t_config_option *xfer_config_color_text_bg = NULL;
struct t_config_option *xfer_config_color_text_selected = NULL;

/* xfer config, network section */

struct t_config_option *xfer_config_network_blocksize = NULL;
struct t_config_option *xfer_config_network_fast_send = NULL;
struct t_config_option *xfer_config_network_own_ip = NULL;
struct t_config_option *xfer_config_network_port_range = NULL;
struct t_config_option *xfer_config_network_send_ack = NULL;
struct t_config_option *xfer_config_network_speed_limit_recv = NULL;
struct t_config_option *xfer_config_network_speed_limit_send = NULL;
struct t_config_option *xfer_config_network_timeout = NULL;

/* xfer config, file section */

struct t_config_option *xfer_config_file_auto_accept_chats = NULL;
struct t_config_option *xfer_config_file_auto_accept_files = NULL;
struct t_config_option *xfer_config_file_auto_accept_nicks = NULL;
struct t_config_option *xfer_config_file_auto_check_crc32 = NULL;
struct t_config_option *xfer_config_file_auto_rename = NULL;
struct t_config_option *xfer_config_file_auto_resume = NULL;
struct t_config_option *xfer_config_file_convert_spaces = NULL;
struct t_config_option *xfer_config_file_download_path = NULL;
struct t_config_option *xfer_config_file_download_temporary_suffix = NULL;
struct t_config_option *xfer_config_file_upload_path = NULL;
struct t_config_option *xfer_config_file_use_nick_in_filename = NULL;


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
xfer_config_init (void)
{
    xfer_config_file = weechat_config_new (XFER_CONFIG_PRIO_NAME,
                                           &xfer_config_reload, NULL, NULL);
    if (!xfer_config_file)
        return 0;

    xfer_config_section_look = weechat_config_new_section (
        xfer_config_file, "look",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (xfer_config_section_look)
    {
        xfer_config_look_auto_open_buffer = weechat_config_new_option (
            xfer_config_file, xfer_config_section_look,
            "auto_open_buffer", "boolean",
            N_("auto open xfer buffer when a new xfer is added "
               "to list"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        xfer_config_look_progress_bar_size = weechat_config_new_option (
            xfer_config_file, xfer_config_section_look,
            "progress_bar_size", "integer",
            N_("size of progress bar, in chars (if 0, progress bar is "
               "disabled)"),
            NULL, 0, XFER_CONFIG_PROGRESS_BAR_MAX_SIZE, "20", NULL, 0,
            NULL, NULL, NULL,
            &xfer_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        xfer_config_look_pv_tags = weechat_config_new_option (
            xfer_config_file, xfer_config_section_look,
            "pv_tags", "string",
            N_("comma separated list of tags used in private messages, for "
               "example: \"notify_message\", \"notify_private\" or "
               "\"notify_highlight\""),
            NULL, 0, 0, "notify_private", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    xfer_config_section_color = weechat_config_new_section (
        xfer_config_file, "color",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (xfer_config_section_color)
    {
        xfer_config_color_status[XFER_STATUS_ABORTED] = weechat_config_new_option (
            xfer_config_file, xfer_config_section_color,
            "status_aborted", "color",
            N_("text color for \"aborted\" status"),
            NULL, 0, 0, "lightred", NULL, 0,
            NULL, NULL, NULL,
            &xfer_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        xfer_config_color_status[XFER_STATUS_ACTIVE] = weechat_config_new_option (
            xfer_config_file, xfer_config_section_color,
            "status_active", "color",
            N_("text color for \"active\" status"),
            NULL, 0, 0, "lightblue", NULL, 0,
            NULL, NULL, NULL,
            &xfer_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        xfer_config_color_status[XFER_STATUS_CONNECTING] = weechat_config_new_option (
            xfer_config_file, xfer_config_section_color,
            "status_connecting", "color",
            N_("text color for \"connecting\" status"),
            NULL, 0, 0, "yellow", NULL, 0,
            NULL, NULL, NULL,
            &xfer_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        xfer_config_color_status[XFER_STATUS_DONE] = weechat_config_new_option (
            xfer_config_file, xfer_config_section_color,
            "status_done", "color",
            N_("text color for \"done\" status"),
            NULL, 0, 0, "lightgreen", NULL, 0,
            NULL, NULL, NULL,
            &xfer_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        xfer_config_color_status[XFER_STATUS_FAILED] = weechat_config_new_option (
            xfer_config_file, xfer_config_section_color,
            "status_failed", "color",
            N_("text color for \"failed\" status"),
            NULL, 0, 0, "lightred", NULL, 0,
            NULL, NULL, NULL,
            &xfer_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        xfer_config_color_status[XFER_STATUS_WAITING] = weechat_config_new_option (
            xfer_config_file, xfer_config_section_color,
            "status_waiting", "color",
            N_("text color for \"waiting\" status"),
            NULL, 0, 0, "lightcyan", NULL, 0,
            NULL, NULL, NULL,
            &xfer_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        xfer_config_color_text = weechat_config_new_option (
            xfer_config_file, xfer_config_section_color,
            "text", "color",
            N_("text color in xfer buffer"),
            NULL, 0, 0, "default", NULL, 0,
            NULL, NULL, NULL,
            &xfer_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        xfer_config_color_text_bg = weechat_config_new_option (
            xfer_config_file, xfer_config_section_color,
            "text_bg", "color",
            N_("background color in xfer buffer"),
            NULL, 0, 0, "default", NULL, 0,
            NULL, NULL, NULL,
            &xfer_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        xfer_config_color_text_selected = weechat_config_new_option (
            xfer_config_file, xfer_config_section_color,
            "text_selected", "color",
            N_("text color of selected line in xfer buffer"),
            NULL, 0, 0, "white", NULL, 0,
            NULL, NULL, NULL,
            &xfer_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
    }

    xfer_config_section_network = weechat_config_new_section (
        xfer_config_file, "network",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (xfer_config_section_network)
    {
        xfer_config_network_blocksize = weechat_config_new_option (
            xfer_config_file, xfer_config_section_network,
            "blocksize", "integer",
            N_("block size for sending packets, in bytes"),
            NULL, XFER_BLOCKSIZE_MIN, XFER_BLOCKSIZE_MAX, "65536", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        xfer_config_network_fast_send = weechat_config_new_option (
            xfer_config_file, xfer_config_section_network,
            "fast_send", "boolean",
            N_("does not wait for ACK when sending file"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        xfer_config_network_own_ip = weechat_config_new_option (
            xfer_config_file, xfer_config_section_network,
            "own_ip", "string",
            N_("IP or DNS address used for sending and passively receiving files/chats "
               "(if empty, local interface IP is used)"),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        xfer_config_network_port_range = weechat_config_new_option (
            xfer_config_file, xfer_config_section_network,
            "port_range", "string",
            N_("restricts outgoing files/chats and incoming/passive files to use "
               "only ports in the given range (useful for NAT) (syntax: a single port, "
               "ie. 5000 or a port range, ie. 5000-5015, empty value means any port, "
               "it's recommended to use ports greater than 1024, because only root "
               "can use ports below 1024)"),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        xfer_config_network_send_ack = weechat_config_new_option (
            xfer_config_file, xfer_config_section_network,
            "send_ack", "boolean",
            N_("send acks when receiving files; if disabled, the transfer may "
               "freeze if the sender is waiting for acks (for example a "
               "WeeChat sending a file with option xfer.network.fast_send set "
               "to off); on the other hand, disabling send of acks may prevent "
               "a freeze if the acks are not sent immediately to the sender"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        xfer_config_network_speed_limit_recv = weechat_config_new_option (
            xfer_config_file, xfer_config_section_network,
            "speed_limit_recv", "integer",
            N_("speed limit for receiving files, in kilo-bytes by second (0 "
               "means no limit)"),
            NULL, 0, INT_MAX, "0", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        xfer_config_network_speed_limit_send = weechat_config_new_option (
            xfer_config_file, xfer_config_section_network,
            "speed_limit_send", "integer",
            N_("speed limit for sending files, in kilo-bytes by second (0 means "
               "no limit)"),
            NULL, 0, INT_MAX, "0", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        xfer_config_network_timeout = weechat_config_new_option (
            xfer_config_file, xfer_config_section_network,
            "timeout", "integer",
            N_("timeout for xfer request (in seconds)"),
            NULL, 5, INT_MAX, "300", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    xfer_config_section_file = weechat_config_new_section (
        xfer_config_file, "file",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (xfer_config_section_file)
    {
        xfer_config_file_auto_accept_chats = weechat_config_new_option (
            xfer_config_file, xfer_config_section_file,
            "auto_accept_chats", "boolean",
            N_("automatically accept chat requests (use carefully!)"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        xfer_config_file_auto_accept_files = weechat_config_new_option (
            xfer_config_file, xfer_config_section_file,
            "auto_accept_files", "boolean",
            N_("automatically accept incoming files (use carefully!)"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        xfer_config_file_auto_accept_nicks = weechat_config_new_option (
            xfer_config_file, xfer_config_section_file,
            "auto_accept_nicks", "string",
            N_("comma-separated list of nicks for which the incoming files and "
               "chats are automatically accepted; format is \"server.nick\" "
               "(for a specific server) or \"nick\" (for all servers); "
               "example: \"libera.FlashCode,andrew\""),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        xfer_config_file_auto_check_crc32 = weechat_config_new_option (
            xfer_config_file, xfer_config_section_file,
            "auto_check_crc32", "boolean",
            N_("automatically check CRC32 file checksum if it is found in the "
               "filename (8 hexadecimal chars)"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        xfer_config_file_auto_rename = weechat_config_new_option (
            xfer_config_file, xfer_config_section_file,
            "auto_rename", "boolean",
            N_("rename incoming files if already exists (add \".1\", \".2\", "
               "...)"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        xfer_config_file_auto_resume = weechat_config_new_option (
            xfer_config_file, xfer_config_section_file,
            "auto_resume", "boolean",
            N_("automatically resume file transfer if connection with remote "
               "host is lost"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        xfer_config_file_convert_spaces = weechat_config_new_option (
            xfer_config_file, xfer_config_section_file,
            "convert_spaces", "boolean",
            N_("convert spaces to underscores when sending and receiving "
               "files"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        xfer_config_file_download_path = weechat_config_new_option (
            xfer_config_file, xfer_config_section_file,
            "download_path", "string",
            N_("path for writing incoming files "
               "(path is evaluated, see function string_eval_path_home in "
               "plugin API reference)"),
            NULL, 0, 0, "${weechat_data_dir}/xfer", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        xfer_config_file_download_temporary_suffix = weechat_config_new_option (
            xfer_config_file, xfer_config_section_file,
            "download_temporary_suffix", "string",
            N_("temporary filename suffix used during the transfer for a file "
               "received, it is removed after successful transfer; "
               "if empty string, no filename suffix is used during the "
               "transfer"),
            NULL, 0, 0, ".part", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        xfer_config_file_upload_path = weechat_config_new_option (
            xfer_config_file, xfer_config_section_file,
            "upload_path", "string",
            N_("path for reading files when sending "
               "(path is evaluated, see function string_eval_path_home in "
               "plugin API reference)"),
            NULL, 0, 0, "~", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        xfer_config_file_use_nick_in_filename = weechat_config_new_option (
            xfer_config_file, xfer_config_section_file,
            "use_nick_in_filename", "boolean",
            N_("use remote nick as prefix in local filename when receiving a "
               "file"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    return 1;
}

/*
 * Reads xfer configuration file.
 */

int
xfer_config_read (void)
{
    return weechat_config_read (xfer_config_file);
}

/*
 * Writes xfer configuration file.
 */

int
xfer_config_write (void)
{
    return weechat_config_write (xfer_config_file);
}
