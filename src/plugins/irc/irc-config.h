/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

#include "../../core/config-option.h"

#define IRC_CONFIG_NAME "irc.rc"

#define CFG_IRC_DISPLAY_AWAY_OFF     0
#define CFG_IRC_DISPLAY_AWAY_LOCAL   1
#define CFG_IRC_DISPLAY_AWAY_CHANNEL 2

extern int irc_cfg_irc_one_server_buffer;
extern int irc_cfg_irc_open_near_server;
extern char *irc_cfg_irc_nick_prefix;
extern char *irc_cfg_irc_nick_suffix;
extern int irc_cfg_irc_display_away;
extern int irc_cfg_irc_show_away_once;
extern char *irc_cfg_irc_default_msg_part;
extern char *irc_cfg_irc_default_msg_quit;
extern int irc_cfg_irc_notice_as_pv;
extern int irc_cfg_irc_away_check;
extern int irc_cfg_irc_away_check_max_nicks;
extern int irc_cfg_irc_lag_check;
extern int irc_cfg_irc_lag_min_show;
extern int irc_cfg_irc_lag_disconnect;
extern int irc_cfg_irc_anti_flood;
extern char *irc_cfg_irc_highlight;
extern int irc_cfg_irc_colors_receive;
extern int irc_cfg_irc_colors_send;
extern int irc_cfg_irc_send_unknown_commands;

extern int irc_cfg_dcc_auto_accept_files;
extern int irc_cfg_dcc_auto_accept_chats;
extern int irc_cfg_dcc_timeout;
extern int irc_cfg_dcc_blocksize;
extern int irc_cfg_dcc_fast_send;
extern char *irc_cfg_dcc_port_range;
extern char *irc_cfg_dcc_own_ip;
extern char *irc_cfg_dcc_download_path;
extern char *irc_cfg_dcc_upload_path;
extern int irc_cfg_dcc_convert_spaces;
extern int irc_cfg_dcc_auto_rename;
extern int irc_cfg_dcc_auto_resume;

extern int irc_cfg_log_auto_server;
extern int irc_cfg_log_auto_channel;
extern int irc_cfg_log_auto_private;
extern int irc_cfg_log_hide_nickserv_pwd;

extern void irc_config_change_noop ();
extern void irc_config_change_one_server_buffer ();
extern void irc_config_change_away_check ();
extern void irc_config_change_log ();
extern void irc_config_change_notify_levels ();
extern int irc_config_read_server (t_config_option *, char *, char *);
extern int irc_config_read ();
extern int irc_config_write_servers (FILE *, char *, t_config_option *);
extern int irc_config_write_servers_default_values (FILE *, char *,
                                                    t_config_option *);
extern int irc_config_write ();

#endif /* irc-config.h */
