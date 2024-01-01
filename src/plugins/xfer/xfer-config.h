/*
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

#ifndef WEECHAT_PLUGIN_XFER_CONFIG_H
#define WEECHAT_PLUGIN_XFER_CONFIG_H

#define XFER_CONFIG_NAME "xfer"
#define XFER_CONFIG_PRIO_NAME (TO_STR(XFER_PLUGIN_PRIORITY) "|" XFER_CONFIG_NAME)

#define XFER_CONFIG_PROGRESS_BAR_MAX_SIZE 256

extern struct t_config_file *xfer_config_file;

extern struct t_config_option *xfer_config_look_auto_open_buffer;
extern struct t_config_option *xfer_config_look_progress_bar_size;
extern struct t_config_option *xfer_config_look_pv_tags;

extern struct t_config_option *xfer_config_color_status[];
extern struct t_config_option *xfer_config_color_text;
extern struct t_config_option *xfer_config_color_text_bg;
extern struct t_config_option *xfer_config_color_text_selected;

extern struct t_config_option *xfer_config_network_blocksize;
extern struct t_config_option *xfer_config_network_fast_send;
extern struct t_config_option *xfer_config_network_own_ip;
extern struct t_config_option *xfer_config_network_port_range;
extern struct t_config_option *xfer_config_network_send_ack;
extern struct t_config_option *xfer_config_network_speed_limit_recv;
extern struct t_config_option *xfer_config_network_speed_limit_send;
extern struct t_config_option *xfer_config_network_timeout;

extern struct t_config_option *xfer_config_file_auto_accept_chats;
extern struct t_config_option *xfer_config_file_auto_accept_files;
extern struct t_config_option *xfer_config_file_auto_accept_nicks;
extern struct t_config_option *xfer_config_file_auto_rename;
extern struct t_config_option *xfer_config_file_auto_resume;
extern struct t_config_option *xfer_config_file_auto_check_crc32;
extern struct t_config_option *xfer_config_file_convert_spaces;
extern struct t_config_option *xfer_config_file_download_path;
extern struct t_config_option *xfer_config_file_download_temporary_suffix;
extern struct t_config_option *xfer_config_file_upload_path;
extern struct t_config_option *xfer_config_file_use_nick_in_filename;

extern int xfer_config_init ();
extern int xfer_config_read ();
extern int xfer_config_write ();

#endif /* WEECHAT_PLUGIN_XFER_CONFIG_H */
