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


#ifndef __WEECHAT_IRC_COMMAND_H
#define __WEECHAT_IRC_COMMAND_H 1

#include "../../core/command.h"

extern t_weechat_command irc_commands[];

extern int irc_cmd_admin (t_gui_window *, char *, int, char **);
extern int irc_cmd_ame (t_gui_window *, char *, int, char **);
extern int irc_cmd_amsg (t_gui_window *, char *, int, char **);
extern int irc_cmd_away (t_gui_window *, char *, int, char **);
extern int irc_cmd_ban (t_gui_window *, char *, int, char **);
extern int irc_cmd_connect (t_gui_window *, char *, int, char **);
extern int irc_cmd_ctcp (t_gui_window *, char *, int, char **);
extern int irc_cmd_cycle (t_gui_window *, char *, int, char **);
extern int irc_cmd_dcc (t_gui_window *, char *, int, char **);
extern int irc_cmd_dehalfop (t_gui_window *, char *, int, char **);
extern int irc_cmd_deop (t_gui_window *, char *, int, char **);
extern int irc_cmd_devoice (t_gui_window *, char *, int, char **);
extern int irc_cmd_die (t_gui_window *, char *, int, char **);
extern int irc_cmd_disconnect (t_gui_window *, char *, int, char **);
extern int irc_cmd_halfop (t_gui_window *, char *, int, char **);
extern int irc_cmd_info (t_gui_window *, char *, int, char **);
extern int irc_cmd_invite (t_gui_window *, char *, int, char **);
extern int irc_cmd_ison (t_gui_window *, char *, int, char **);
extern int irc_cmd_join (t_gui_window *, char *, int, char **);
extern int irc_cmd_kick (t_gui_window *, char *, int, char **);
extern int irc_cmd_kickban (t_gui_window *, char *, int, char **);
extern int irc_cmd_kill (t_gui_window *, char *, int, char **);
extern int irc_cmd_links (t_gui_window *, char *, int, char **);
extern int irc_cmd_list (t_gui_window *, char *, int, char **);
extern int irc_cmd_lusers (t_gui_window *, char *, int, char **);
extern int irc_cmd_me (t_gui_window *, char *, int, char **);
extern int irc_cmd_mode (t_gui_window *, char *, int, char **);
extern int irc_cmd_motd (t_gui_window *, char *, int, char **);
extern int irc_cmd_msg (t_gui_window *, char *, int, char **);
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
extern int irc_cmd_reconnect (t_gui_window *, char *, int, char **);
extern int irc_cmd_rehash (t_gui_window *, char *, int, char **);
extern int irc_cmd_restart (t_gui_window *, char *, int, char **);
extern int irc_cmd_service (t_gui_window *, char *, int, char **);
extern int irc_cmd_server (t_gui_window *, char *, int, char **);
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

#endif /* irc-command.h */
