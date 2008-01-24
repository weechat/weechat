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


#ifndef __WEECHAT_IRC_PROTOCOL_H
#define __WEECHAT_IRC_PROTOCOL_H 1

#include "irc-server.h"

typedef int (t_irc_recv_func)(struct t_irc_server *server, char *irc_message,
                              char *host, char *nick, char *arguments,
                              int ignore, int highlight);
typedef int (t_irc_recv_func2)(struct t_irc_server *server, int argc,
                               char **argv, char **argv_eol,
                               int ignore, int highlight);

struct t_irc_protocol_msg
{
    char *name;                     /* IRC message name                      */
    char *description;              /* message description                   */
    t_irc_recv_func *recv_function; /* function called when msg is received  */
    t_irc_recv_func2 *recv_function2; /* function called when msg is received  */
};

extern int irc_protocol_is_highlight (char *message, char *nick);
extern int irc_protocol_recv_command (struct t_irc_server *server, char *entire_line, char *host, char *command, char *arguments);
extern int irc_protocol_cmd_error (struct t_irc_server *server, int argc, char **argv, char **argv_eol, int ignore, int highlight);
extern int irc_protocol_cmd_invite (struct t_irc_server *server, int argc, char **argv, char **argv_eol, int ignore, int highlight);
extern int irc_protocol_cmd_join (struct t_irc_server *server, int argc, char **argv, char **argv_eol, int ignore, int highlight);
extern int irc_protocol_cmd_kick (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_kill (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_mode (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_nick (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_notice (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_part (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_ping (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_pong (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_privmsg (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_quit (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_server_mode_reason (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_server_msg (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_server_reply (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_topic (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_wallops (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_001 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_005 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_221 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_301 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_302 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_303 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_305 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_306 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_whois_nick_msg (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_310 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_311 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_312 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_314 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_315 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_317 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_319 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_321 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_322 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_323 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_324 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_327 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_329 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_331 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_332 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_333 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_338 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_341 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_344 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_345 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_348 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_349 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_351 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_352 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_353 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_365 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_366 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_367 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_368 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_432 (struct t_irc_server *server, int argc, char **argv, char **argv_eol, int ignore, int highlight);
extern int irc_protocol_cmd_433 (struct t_irc_server *server, int argc, char **argv, char **argv_eol, int ignore, int highlight);
extern int irc_protocol_cmd_438 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);
extern int irc_protocol_cmd_671 (struct t_irc_server *server, char *irc_message, char *host, char *nick, char *arguments, int ignore, int highlight);

#endif /* irc-protocol.h */
