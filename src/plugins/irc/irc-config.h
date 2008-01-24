/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __WEECHAT_IRC_CONFIG_H
#define __WEECHAT_IRC_CONFIG_H 1

#define IRC_CONFIG_FILENAME "irc.rc"

#define IRC_CONFIG_DISPLAY_AWAY_OFF     0
#define IRC_CONFIG_DISPLAY_AWAY_LOCAL   1
#define IRC_CONFIG_DISPLAY_AWAY_CHANNEL 2

struct t_config_file *irc_config;

struct t_config_option *irc_config_irc_one_server_buffer;
struct t_config_option *irc_config_irc_open_near_server;
struct t_config_option *irc_config_irc_nick_prefix;
struct t_config_option *irc_config_irc_nick_suffix;
struct t_config_option *irc_config_irc_nick_completion_smart;
struct t_config_option *irc_config_irc_display_away;
struct t_config_option *irc_config_irc_show_away_once;
struct t_config_option *irc_config_irc_default_msg_part;
struct t_config_option *irc_config_irc_default_msg_quit;
struct t_config_option *irc_config_irc_notice_as_pv;
struct t_config_option *irc_config_irc_away_check;
struct t_config_option *irc_config_irc_away_check_max_nicks;
struct t_config_option *irc_config_irc_lag_check;
struct t_config_option *irc_config_irc_lag_min_show;
struct t_config_option *irc_config_irc_lag_disconnect;
struct t_config_option *irc_config_irc_anti_flood;
struct t_config_option *irc_config_irc_highlight;
struct t_config_option *irc_config_irc_colors_receive;
struct t_config_option *irc_config_irc_colors_send;
struct t_config_option *irc_config_irc_send_unknown_commands;

struct t_config_option *irc_config_dcc_auto_accept_files;
struct t_config_option *irc_config_dcc_auto_accept_chats;
struct t_config_option *irc_config_dcc_timeout;
struct t_config_option *irc_config_dcc_blocksize;
struct t_config_option *irc_config_dcc_fast_send;
struct t_config_option *irc_config_dcc_port_range;
struct t_config_option *irc_config_dcc_own_ip;
struct t_config_option *irc_config_dcc_download_path;
struct t_config_option *irc_config_dcc_upload_path;
struct t_config_option *irc_config_dcc_convert_spaces;
struct t_config_option *irc_config_dcc_auto_rename;
struct t_config_option *irc_config_dcc_auto_resume;

struct t_config_option *irc_config_log_auto_server;
struct t_config_option *irc_config_log_auto_channel;
struct t_config_option *irc_config_log_auto_private;
struct t_config_option *irc_config_log_hide_nickserv_pwd;

int irc_config_init ();
int irc_config_read ();
int irc_config_write ();

#endif /* irc-config.h */
