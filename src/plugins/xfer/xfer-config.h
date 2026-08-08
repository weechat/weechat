/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

extern int xfer_config_init (void);
extern int xfer_config_read (void);
extern int xfer_config_write (void);

#endif /* WEECHAT_PLUGIN_XFER_CONFIG_H */
