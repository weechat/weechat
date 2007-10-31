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


#ifndef __WEECHAT_IRC_H
#define __WEECHAT_IRC_H 1

#include "irc-buffer.h"
#include "irc-color.h"
#include "irc-command.h"
#include "irc-config.h"
#include "irc-server.h"
#include "irc-channel.h"
#include "irc-nick.h"
#include "irc-dcc.h"
#include "irc-protocol.h"

#include "../protocol.h"

#define _PROTOCOL_NAME    "irc"
#define _PROTOCOL_VERSION "0.1"
#define _PROTOCOL_DESC    "IRC (Internet Relay Chat)"

extern t_weechat_protocol *irc_protocol;
extern t_weechat_hook *irc_hook_timer, *irc_hook_timer_check_away;

/* buffer functions (irc-buffer.c) */

extern t_irc_buffer_data *irc_buffer_data_create (t_irc_server *);
extern void irc_buffer_data_free (t_gui_buffer *);
extern void irc_buffer_merge_servers (t_gui_window *);
extern void irc_buffer_split_server (t_gui_window *);

/* channel functions (irc-channel.c) */

extern t_irc_channel *irc_channel_new (t_irc_server *, int, char *, int);
extern void irc_channel_free (t_irc_server *, t_irc_channel *);
extern void irc_channel_free_all (t_irc_server *);
extern t_irc_channel *irc_channel_search (t_irc_server *, char *);
extern t_irc_channel *irc_channel_search_any (t_irc_server *, char *);
extern t_irc_channel *irc_channel_search_any_without_buffer (t_irc_server *, char *);
extern t_irc_channel *irc_channel_search_dcc (t_irc_server *, char *);
extern int irc_channel_is_channel (char *);
extern void irc_channel_remove_away (t_irc_channel *);
extern void irc_channel_check_away (t_irc_server *, t_irc_channel *, int);
extern void irc_channel_set_away (t_irc_channel *, char *, int);
extern int irc_channel_create_dcc (t_irc_dcc *);
extern int irc_channel_get_notify_level (t_irc_server *, t_irc_channel *);
extern void irc_channel_set_notify_level (t_irc_server *, t_irc_channel *, int);
extern void irc_channel_add_nick_speaking (t_irc_channel *, char *);
extern void irc_channel_print_log (t_irc_channel *);

/* color functions (irc-color.c) */

extern unsigned char *irc_color_decode (unsigned char *, int, int);
extern unsigned char *irc_color_decode_for_user_entry (unsigned char *);
extern unsigned char *irc_color_encode (unsigned char *, int);

/* IRC commands (irc-command.c) */

extern int irc_cmd_admin (t_gui_window *, char *, int, char **);
extern void irc_cmd_mode_nicks (t_irc_server *, char *, char *, char *, int, char **);
extern int irc_cmd_ame (t_gui_window *, char *, int, char **);
extern int irc_cmd_amsg (t_gui_window *, char *, int, char **);
extern void irc_cmd_away_server (t_irc_server *, char *);
extern int irc_cmd_away (t_gui_window *, char *, int, char **);
extern int irc_cmd_ban (t_gui_window *, char *, int, char **);
extern int irc_cmd_ctcp (t_gui_window *, char *, int, char **);
extern int irc_cmd_cycle (t_gui_window *, char *, int, char **);
extern int irc_cmd_dehalfop (t_gui_window *, char *, int, char **);
extern int irc_cmd_deop (t_gui_window *, char *, int, char **);
extern int irc_cmd_devoice (t_gui_window *, char *, int, char **);
extern int irc_cmd_die (t_gui_window *, char *, int, char **);
extern int irc_cmd_halfop (t_gui_window *, char *, int, char **);
extern int irc_cmd_info (t_gui_window *, char *, int, char **);
extern int irc_cmd_invite (t_gui_window *, char *, int, char **);
extern int irc_cmd_ison (t_gui_window *, char *, int, char **);
extern void irc_cmd_join_server (t_irc_server *, char *);
extern int irc_cmd_join (t_gui_window *, char *, int, char **);
extern int irc_cmd_kick (t_gui_window *, char *, int, char **);
extern int irc_cmd_kickban (t_gui_window *, char *, int, char **);
extern int irc_cmd_kill (t_gui_window *, char *, int, char **);
extern int irc_cmd_links (t_gui_window *, char *, int, char **);
extern int irc_cmd_list (t_gui_window *, char *, int, char **);
extern int irc_cmd_lusers (t_gui_window *, char *, int, char **);
extern int irc_cmd_me (t_gui_window *, char *, int, char **);
extern void irc_cmd_mode_server (t_irc_server *, char *);
extern int irc_cmd_mode (t_gui_window *, char *, int, char **);
extern int irc_cmd_motd (t_gui_window *, char *, int, char **);
extern int irc_cmd_msg (t_gui_window *, char *, int, char **);
extern int irc_cmd_names (t_gui_window *, char *, int, char **);
extern int irc_cmd_nick (t_gui_window *, char *, int, char **);
extern int irc_cmd_notice (t_gui_window *, char *, int, char **);
extern int irc_cmd_op (t_gui_window *, char *, int, char **);
extern int irc_cmd_oper (t_gui_window *, char *, int, char **);
extern int irc_cmd_part (t_gui_window *, char *, int, char **);
extern int irc_cmd_ping (t_gui_window *, char *, int, char **);
extern int irc_cmd_pong (t_gui_window *, char *, int, char **);
extern int irc_cmd_query (t_gui_window *, char *, int, char **);
extern int irc_cmd_quit (t_gui_window *, char *, int, char **);
extern int irc_cmd_quote (t_gui_window *, char *, int, char **);
extern int irc_cmd_rehash (t_gui_window *, char *, int, char **);
extern int irc_cmd_restart (t_gui_window *, char *, int, char **);
extern int irc_cmd_service (t_gui_window *, char *, int, char **);
extern int irc_cmd_servlist (t_gui_window *, char *, int, char **);
extern int irc_cmd_squery (t_gui_window *, char *, int, char **);
extern int irc_cmd_squit (t_gui_window *, char *, int, char **);
extern int irc_cmd_stats (t_gui_window *, char *, int, char **);
extern int irc_cmd_summon (t_gui_window *, char *, int, char **);
extern int irc_cmd_time (t_gui_window *, char *, int, char **);
extern int irc_cmd_topic (t_gui_window *, char *, int, char **);
extern int irc_cmd_trace (t_gui_window *, char *, int, char **);
extern int irc_cmd_unban (t_gui_window *, char *, int, char **);
extern int irc_cmd_userhost (t_gui_window *, char *, int, char **);
extern int irc_cmd_users (t_gui_window *, char *, int, char **);
extern int irc_cmd_version (t_gui_window *, char *, int, char **);
extern int irc_cmd_voice (t_gui_window *, char *, int, char **);
extern int irc_cmd_wallops (t_gui_window *, char *, int, char **);
extern int irc_cmd_who (t_gui_window *, char *, int, char **);
extern int irc_cmd_whois (t_gui_window *, char *, int, char **);
extern int irc_cmd_whowas (t_gui_window *, char *, int, char **);

/* config functions (irc-config.c) */

extern void irc_config_create_dirs ();
extern void *irc_config_get_server_option_ptr (t_irc_server *, char *);
extern int irc_config_set_server_value (t_irc_server *, char *, char *);
extern int irc_config_read ();
extern int irc_config_write ();

/* DCC functions (irc-dcc.c) */

extern void irc_dcc_redraw (int);
extern void irc_dcc_free (t_irc_dcc *);
extern void irc_dcc_close (t_irc_dcc *, int);
extern void irc_dcc_chat_remove_channel (t_irc_channel *);
extern void irc_dcc_accept (t_irc_dcc *);
extern void irc_dcc_accept_resume (t_irc_server *, char *, int, unsigned long);
extern void irc_dcc_start_resume (t_irc_server *, char *, int, unsigned long);
extern t_irc_dcc *irc_dcc_alloc ();
extern t_irc_dcc *irc_dcc_add (t_irc_server *, int, unsigned long, int, char *, int,
                               char *, char *, unsigned long);
extern void irc_dcc_send_request (t_irc_server *, int, char *, char *);
extern void irc_dcc_chat_sendf (t_irc_dcc *, char *, ...);
extern void irc_dcc_file_send_fork (t_irc_dcc *);
extern void irc_dcc_file_recv_fork (t_irc_dcc *);
extern void irc_dcc_handle ();
extern void irc_dcc_end ();
extern void irc_dcc_print_log ();

/* display functions (irc-diplay.c) */

extern void irc_display_hide_password (char *, int);
extern void irc_display_nick (t_gui_buffer *, t_irc_nick *, char *, int,
                              int, char *, int);
extern void irc_display_away (t_irc_server *, char *, char *);
extern void irc_display_mode (t_gui_buffer *, char *, char *,
                              char, char *, char *, char *, char *);
extern void irc_display_server (t_irc_server *ptr_server, int);

/* input functions (irc-input.c) */

extern int irc_input_data (t_gui_window *, char *);

/* log functions (irc-log.c) */
extern char *irc_log_get_filename (char *, char *, int);

/* mode functions (irc-mode.c) */

extern void irc_mode_channel_set (t_irc_server *, t_irc_channel *, char *);
extern void irc_mode_user_set (t_irc_server *, char *);
extern int irc_mode_nick_prefix_allowed (t_irc_server *, char);

/* nick functions (irc-nick.c) */

extern int irc_nick_find_color (t_irc_nick *);
extern void irc_nick_get_gui_infos (t_irc_nick *, int *, char *, int *);
extern t_irc_nick *irc_nick_new (t_irc_server *, t_irc_channel *, char *,
                                 int, int, int, int, int, int, int);
extern void irc_nick_change (t_irc_server *, t_irc_channel *, t_irc_nick *, char *);
extern void irc_nick_free (t_irc_channel *, t_irc_nick *);
extern void irc_nick_free_all (t_irc_channel *);
extern t_irc_nick *irc_nick_search (t_irc_channel *, char *);
extern void irc_nick_count (t_irc_channel *, int *, int *, int *, int *, int *);
extern void irc_nick_set_away (t_irc_channel *, t_irc_nick *, int);
extern void irc_nick_print_log (t_irc_nick *);

/* IRC protocol (irc-protocol.c) */

extern int irc_protocol_is_highlight (char *, char *);
extern int irc_protocol_recv_command (t_irc_server *, char *, char *, char *, char *);
extern int irc_protocol_cmd_error (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_invite (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_join (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_kick (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_kill (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_mode (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_nick (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_notice (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_part (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_ping (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_pong (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_privmsg (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_quit (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_server_mode_reason (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_server_msg (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_server_reply (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_topic (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_wallops (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_001 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_005 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_221 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_301 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_302 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_303 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_305 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_306 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_whois_nick_msg (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_310 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_311 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_312 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_314 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_315 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_317 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_319 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_321 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_322 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_323 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_324 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_327 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_329 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_331 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_332 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_333 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_338 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_341 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_344 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_345 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_348 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_349 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_351 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_352 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_353 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_365 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_366 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_367 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_368 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_432 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_433 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_438 (t_irc_server *, char *, char *, char *, char *, int, int);
extern int irc_protocol_cmd_671 (t_irc_server *, char *, char *, char *, char *, int, int);

/* server functions (irc-server.c) */

extern void irc_server_init (t_irc_server *);
extern int irc_server_init_with_url (char *, t_irc_server *);
extern t_irc_server *irc_server_alloc ();
extern void irc_server_outqueue_free_all (t_irc_server *);
extern void irc_server_destroy (t_irc_server *);
extern void irc_server_free (t_irc_server *);
extern void irc_server_free_all ();
extern t_irc_server *irc_server_new (char *, int, int, int, int, char *, int, int, int,
                                     char *, char *, char *, char *, char *, char *,
                                     char *, char *, int, char *, int, char *);
extern t_irc_server *irc_server_duplicate (t_irc_server *, char *);
extern int irc_server_rename (t_irc_server *, char *);
extern int irc_server_send (t_irc_server *, char *, int);
extern void irc_server_outqueue_send (t_irc_server *);
extern void irc_server_sendf (t_irc_server *, char *, ...);
extern void irc_server_parse_message (char *, char **, char **, char **);
extern void irc_server_recv (void *);
extern void irc_server_timer (void *);
extern void irc_server_timer_check_away (void *);
extern void irc_server_child_read (void *);
extern void irc_server_convbase64_8x3_to_6x4 (char *, char*);
extern void irc_server_base64encode (char *, char *);
extern int irc_server_pass_httpproxy (int, char*, int);
extern int irc_server_resolve (char *, char *, int *);
extern int irc_server_pass_socks4proxy (int, char*, int, char*);
extern int irc_server_pass_socks5proxy (int, char*, int);
extern int irc_server_pass_proxy (int, char*, int, char*);
extern int irc_server_connect (t_irc_server *, int);
extern void irc_server_reconnect (t_irc_server *);
extern void irc_server_auto_connect (int, int);
extern void irc_server_disconnect (t_irc_server *, int);
extern void irc_server_disconnect_all ();
extern void irc_server_autojoin_channels ();
extern t_irc_server *irc_server_search (char *);
extern int irc_server_get_number_connected ();
extern void irc_server_get_number_buffer (t_irc_server *, int *, int *);
extern int irc_server_name_already_exists (char *);
extern int irc_server_get_channel_count (t_irc_server *);
extern int irc_server_get_pv_count (t_irc_server *);
extern void irc_server_remove_away ();
extern void irc_server_check_away ();
extern void irc_server_set_away (t_irc_server *, char *, int);
extern int irc_server_get_default_notify_level (t_irc_server *);
extern void irc_server_set_default_notify_level (t_irc_server *, int);
extern void irc_server_print_log (t_irc_server *);

#endif /* irc.h */
