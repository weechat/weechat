/*
 * Copyright (c) 2003 by FlashCode <flashcode@flashtux.org>
 *                       Bounga <bounga@altern.org>
 *                       Xahlexx <xahlexx@tuxisland.org>
 * See README for License detail.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


/* irc-commands.c: implementation of IRC commands, according to
                   RFC 1459,2810,2811,2812 */


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/utsname.h>

#include "../weechat.h"
#include "irc.h"
#include "../command.h"
#include "../config.h"
#include "../gui/gui.h"


t_irc_command irc_commands[] =
{ { "away", N_("toggle away status"),
    N_("[-all] [message]"),
    N_("-all: toggle away status on all connected servers\n"
    "message: message for away (if no message is given, away status is removed)"),
    0, MAX_ARGS, 1, NULL, irc_cmd_send_away, NULL },
  { "ctcp", N_("send a ctcp message"),
    N_("nickname type"),
    N_("nickname: user to send ctcp to\ntype: \"action\" or \"version\""),
    2, MAX_ARGS, 1, NULL, irc_cmd_send_ctcp, NULL },
  { "deop", N_("removes channel operator status from nickname(s)"),
    N_("nickname [nickname]"), "",
    1, 1, 1, irc_cmd_send_deop, NULL, NULL },
  { "devoice", N_("removes voice from nickname(s)"),
    N_("nickname [nickname]"), "",
    1, 1, 1, irc_cmd_send_devoice, NULL, NULL },
  { "error", N_("error received from IRC server"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_error },
  { "invite", N_("invite a nick on a channel"),
    N_("nickname channel"),
    N_("nickname: nick to invite\nchannel: channel to invite"),
    2, 2, 1, NULL, irc_cmd_send_invite, NULL },
  { "join", N_("join a channel"),
    N_("channel[,channel] [key[,key]]"),
    N_("channel: channel name to join\nkey: key to join the channel"),
    1, MAX_ARGS, 1, NULL, irc_cmd_send_join, irc_cmd_recv_join },
  { "kick", N_("forcibly remove a user from a channel"),
    N_("[channel] nickname [comment]"),
    N_("channel: channel where user is\nnickname: nickname to kick\ncomment: comment for kick"),
    1, MAX_ARGS, 1, NULL, irc_cmd_send_kick, irc_cmd_recv_kick },
  { "kill", N_("close client-server connection"),
    N_("nickname comment"),
    N_("nickname: nickname\ncomment: comment for kill"),
    2, MAX_ARGS, 1, NULL, irc_cmd_send_kill, NULL },
  { "list", N_("list channels and their topic"),
    N_("[channel[,channel] [server]]"),
    N_("channel: channel to list\nserver: server name"),
    0, MAX_ARGS, 1, NULL, irc_cmd_send_list, NULL },
  { "me", N_("send a ctcp action to the current channel"),
    N_("message"),
    N_("message: message to send"),
    1, MAX_ARGS, 1, NULL, irc_cmd_send_me, NULL },
  { "mode", N_("change channel or user mode"),
    N_("{ channel {[+|-]|o|p|s|i|t|n|b|v} [limit] [user] [ban mask] } | "
    "{ nickname {[+|-]|i|w|s|o}"),
    N_("channel modes:\n"
    "  channel: channel name to modify\n"
    "  o: give/take channel operator privileges\n"
    "  p: private channel flag\n"
    "  s: secret channel flag\n"
    "  i: invite-only channel flag\n"
    "  t: topic settable by channel operator only flag\n"
    "  n: no messages to channel from clients on the outside\n"
    "  m: moderated channel\n"
    "  l: set the user limit to channel\n"
    "  b: set a ban mask to keep users out\n"
    "  v: give/take the ability to speak on a moderated channel\n"
    "  k: set a channel key (password)\n"
    "user modes:\n"
    "  nickname: nickname to modify\n"
    "  i: mark a user as invisible\n"
    "  s: mark a user for receive server notices\n"
    "  w: user receives wallops\n"
    "  o: operator flag\n"),
    1, MAX_ARGS, 1, NULL, irc_cmd_send_mode, irc_cmd_recv_mode },
  { "msg", N_("send message to a nick or channel"),
    N_("receiver[,receiver] text"), N_("receiver: nick or channel (may be mask)"
    "\ntext: text to send"),
    1, MAX_ARGS, 1, NULL, irc_cmd_send_msg, NULL },
  { "names", N_("list nicknames on channels"),
    N_("[channel[,channel]]"), N_("channel: channel name"),
    0, MAX_ARGS, 1, NULL, irc_cmd_send_names, NULL },
  { "nick", N_("change current nickname"),
    N_("nickname"), N_("nickname: new nickname for current IRC server"),
    1, 1, 1, irc_cmd_send_nick, NULL, irc_cmd_recv_nick },
  { "notice", N_("send notice message to user"),
    N_("nickname text"), N_("nickname: user to send notice to\ntext: text to send"),
    1, MAX_ARGS, 1, NULL, irc_cmd_send_notice, irc_cmd_recv_notice },
  { "op", N_("gives channel operator status to nickname(s)"),
    N_("nickname [nickname]"), "",
    1, 1, 1, irc_cmd_send_op, NULL, NULL },
  { "oper", N_("get operator privileges"),
    N_("user password"),
    N_("user/password: used to get privileges on current IRC server"),
    2, 2, 1, irc_cmd_send_oper, NULL, NULL },
  { "part", N_("leave a channel"),
    N_("[channel[,channel]]"), N_("channel: channel name to join"),
    0, MAX_ARGS, 1, NULL, irc_cmd_send_part, irc_cmd_recv_part },
  { "ping", N_("ping server"),
    N_("server1 [server2]"),
    N_("server1: server to ping\nserver2: forward ping to this server"),
    1, 2, 1, irc_cmd_send_ping, NULL, irc_cmd_recv_ping },
  { "pong", N_("answer to a ping message"),
    N_("daemon [daemon2]"), N_("daemon: daemon who has responded to Ping message\n"
    "daemon2: forward message to this daemon"),
    1, 2, 1, irc_cmd_send_pong, NULL, NULL },
  { "privmsg", N_("message received"),
    "", "",
    0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_privmsg },
  { "quit", N_("close all connections & quit " WEECHAT_NAME),
    N_("[quit_message]"),
    N_("quit_message: quit message (displayed to other users)"),
    0, MAX_ARGS, 0, NULL, irc_cmd_send_quit, irc_cmd_recv_quit },
  { "quote", N_("send raw data to server without parsing"),
    N_("data"),
    N_("data: raw data to send"),
    1, MAX_ARGS, 1, NULL, irc_cmd_send_quote, NULL },
  { "topic", N_("get/set channel topic"),
    N_("[channel] [topic]"), N_("channel: channel name\ntopic: new topic for channel "
    "(if topic is \"-delete\" then topic is deleted)"),
    0, MAX_ARGS, 1, NULL, irc_cmd_send_topic, irc_cmd_recv_topic },
  { "version", N_("gives the version info of nick or server (current or specified)"),
    N_("[server | nickname]"), N_("server: server name\nnickname: nickname"),
    0, 1, 1, NULL, irc_cmd_send_version, NULL },
  { "voice", N_("gives voice to nickname(s)"),
    N_("nickname [nickname]"), "",
    1, 1, 1, irc_cmd_send_voice, NULL, NULL },
  { "whois", N_("query information about user(s)"),
    N_("[server] nickname[,nickname]"), N_("server: server name\n"
    "nickname: nickname (may be a mask)"),
    1, MAX_ARGS, 1, NULL, irc_cmd_send_whois, NULL },
  { "001", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "002", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "003", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "004", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_004 },
  { "005", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "250", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "251", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "252", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "253", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "254", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "255", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "256", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "257", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "258", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "259", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "260", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "261", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "262", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "263", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "264", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "265", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "266", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "267", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "268", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "269", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "301", N_("away message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_301 },
  { "305", N_("unaway"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_reply },
  { "306", N_("now away"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_reply },
  { "311", N_("whois (user)"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_311 },
  { "312", N_("whois (server)"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_312 },
  { "313", N_("whois (operator)"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_313 },
  { "317", N_("whois (idle)"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_317 },
  { "318", N_("whois (end)"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_318 },
  { "319", N_("whois (channels)"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_319 },
  { "320", N_("whois (identified user)"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_320 },
  { "321", N_("/list start"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_321 },  
  { "322", N_("channel (for /list)"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_322 },
  { "323", N_("/list end"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_323 },
  { "331", N_("no topic for channel"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_331 },
  { "332", N_("topic of channel"),
    N_("channel :topic"),
    N_("channel: name of channel\ntopic: topic of the channel"),
    2, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_332 },
  { "333", N_("infos about topic (nick & date changed)"),
    "", "",
    0, 0, 1, NULL, NULL, irc_cmd_recv_333 },
  { "351", N_("server version"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_351 },
  { "353", N_("list of nicks on channel"),
    N_("channel :[[@|+]nick ...]"),
    N_("channel: name of channel\nnick: nick on the channel"),
    2, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_353 },
  { "366", N_("end of /names list"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_366 },
  { "371", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "372", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "373", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "374", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "375", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "376", N_("a server message"), "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_server_msg },
  { "401", N_("no such nick/channel"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "402", N_("no such server"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "403", N_("no such channel"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "404", N_("cannot send to channel"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "405", N_("too many channels"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "406", N_("was no such nick"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "406", N_("was no such nick"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "407", N_("was no such nick"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "409", N_("no origin"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "411", N_("no recipient"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "412", N_("no text to send"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "413", N_("no toplevel"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "414", N_("wilcard in toplevel domain"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "421", N_("unknown command"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "422", N_("MOTD is missing"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "423", N_("no administrative info"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "424", N_("file error"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "431", N_("no nickname given"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "432", N_("erroneus nickname"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "433", N_("nickname already in use"),
    "", "", 0, 0, 1, NULL, NULL, irc_cmd_recv_433 },
  { "436", N_("nickname collision"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "441", N_("user not in channel"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "442", N_("not on channel"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "443", N_("user already on channel"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "444", N_("user not logged in"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "445", N_("summon has been disabled"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "446", N_("users has been disabled"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "451", N_("you are not registered"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "461", N_("not enough parameters"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "462", N_("you may not register"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "463", N_("your host isn't among the privileged"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "464", N_("password incorrect"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "465", N_("you are banned from this server"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "467", N_("channel key already set"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "471", N_("channel is already full"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "472", N_("unknown mode char to me"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "473", N_("cannot join channel (invite only)"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "474", N_("cannot join channel (banned from channel)"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "475", N_("cannot join channel (bad channel key)"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "481", N_("you're not an IRC operator"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "482", N_("you're not channel operator"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "483", N_("you can't kill a server!"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "491", N_("no O-lines for your host"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "501", N_("unknown mode flag"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { "502", N_("can't change mode for other users"),
    "", "", 0, MAX_ARGS, 1, NULL, NULL, irc_cmd_recv_error },
  { NULL, NULL, NULL, NULL, 0, 0, 1, NULL, NULL, NULL }
};


/*
 * irc_recv_command: executes action when receiving IRC command
 *                   returns:  0 = all ok, command executed
 *                            -1 = command failed
 *                            -2 = no command to execute
 *                            -3 = command not found
 */

int
irc_recv_command (t_irc_server *server,
                  char *host, char *command, char *arguments)
{
    int i, cmd_found;

    #if DEBUG >= 2
    gui_printf (server->window, "recv_irc_command: cmd=%s args=%s\n",
                command, arguments);
    #endif
    
    if (command == NULL)
        return -2;

    /* looks for irc command */
    cmd_found = -1;
    for (i = 0; irc_commands[i].command_name; i++)
        if (strcasecmp (irc_commands[i].command_name, command) == 0)
        {
            cmd_found = i;
            break;
        }

    /* command not found */
    if (cmd_found < 0)
        return -3;

    if (irc_commands[i].recv_function != NULL)
        return (int) (irc_commands[i].recv_function) (server, host, arguments);
    
    return 0;
}

/*
 * irc_login: login to irc server
 */

void
irc_login (t_irc_server *server)
{
    char hostname[128];

    if ((server->password) && (server->password[0]))
        server_sendf (server, "PASS %s\r\n", server->password);
    
    gethostname (hostname, sizeof (hostname) - 1);
    hostname[sizeof (hostname) - 1] = '\0';
    if (!hostname[0])
        strcpy (hostname, _("unknown"));
    gui_printf (server->window,
                _(WEECHAT_NAME ": using local hostname \"%s\"\n"),
                hostname);
    server_sendf (server,
                  "NICK %s\r\n"
                  "USER %s %s %s :%s\r\n",
                  server->nick, server->username, hostname, "servername",
                  server->realname);
}

/*
 * irc_cmd_send_away: toggle away status
 */

int
irc_cmd_send_away (t_irc_server *server, char *arguments)
{
    char *pos;
    t_irc_server *ptr_server;
    
    if (arguments && (strncmp (arguments, "-all", 4) == 0))
    {
        pos = arguments + 4;
        while (pos[0] == ' ')
            pos++;
        if (!pos[0])
            pos = NULL;
        
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (server->is_connected)
            {
                if (pos)
                    server_sendf (ptr_server, "AWAY :%s\r\n", pos);
                else
                    server_sendf (ptr_server, "AWAY\r\n");
            }
        }
    }
    else
    {
        if (arguments)
            server_sendf (server, "AWAY :%s\r\n", arguments);
        else
            server_sendf (server, "AWAY\r\n");
    }
    return 0;
}

/*
 * irc_cmd_send_ctcp: send a ctcp message
 */

int
irc_cmd_send_ctcp (t_irc_server *server, char *arguments)
{
    char *pos, *pos2;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
        pos2 = strchr (pos, ' ');
        if (pos2)
        {
            pos2[0] = '\0';
            pos2++;
            while (pos2[0] == ' ')
                pos2++;
        }
        else
            pos2 = NULL;
        
        if (strcasecmp (pos, "version") == 0)
        {
            if (pos2)
                server_sendf (server, "PRIVMSG %s :\01VERSION %s\01\r\n",
                              arguments, pos2);
            else
                server_sendf (server, "PRIVMSG %s :\01VERSION\01\r\n",
                              arguments);
        }
        if (strcasecmp (pos, "action") == 0)
        {
            if (pos2)
                server_sendf (server, "PRIVMSG %s :\01ACTION %s\01\r\n",
                              arguments, pos2);
            else
                server_sendf (server, "PRIVMSG %s :\01ACTION\01\r\n",
                              arguments);
        }
        
    }
    return 0;
}

/*
 * irc_cmd_send_deop: remove operator privileges from nickname(s)
 */

int
irc_cmd_send_deop (t_irc_server *server, int argc, char **argv)
{
    int i;
    
    if (WIN_IS_CHANNEL(gui_current_window))
    {
        for (i = 0; i < argc; i++)
            server_sendf (server, "MODE %s -o %s\r\n",
                          CHANNEL(gui_current_window)->name,
                          argv[i]);
    }
    else
        gui_printf (server->window,
                    _("%s \"deop\" command can only be executed in a channel window\n"),
                    WEECHAT_ERROR);
    return 0;
}

/*
 * irc_cmd_send_devoice: remove voice from nickname(s)
 */

int
irc_cmd_send_devoice (t_irc_server *server, int argc, char **argv)
{
    int i;
    
    if (WIN_IS_CHANNEL(gui_current_window))
    {
        for (i = 0; i < argc; i++)
            server_sendf (server, "MODE %s -v %s\r\n",
                          CHANNEL(gui_current_window)->name,
                          argv[i]);
    }
    else
    {
        gui_printf (server->window,
                    _("%s \"devoice\" command can only be executed in a channel window\n"),
                    WEECHAT_ERROR);
        return -1;
    }
    return 0;
}

/*
 * irc_cmd_send_invite: invite a nick on a channel
 */

int
irc_cmd_send_invite (t_irc_server *server, char *arguments)
{
    server_sendf (server, "INVITE %s\r\n", arguments);
    return 0;
}

/*
 * irc_cmd_send_join: join a new channel
 */

int
irc_cmd_send_join (t_irc_server *server, char *arguments)
{
    server_sendf (server, "JOIN %s\r\n", arguments);
    return 0;
}

/*
 * irc_cmd_send_kick: forcibly remove a user from a channel
 */

int
irc_cmd_send_kick (t_irc_server *server, char *arguments)
{
    if (string_is_channel (arguments))
        server_sendf (server, "KICK %s\r\n", arguments);
    else
    {
        if (WIN_IS_CHANNEL (gui_current_window))
        {
            server_sendf (server,
                          "KICK %s %s\r\n",
                          CHANNEL(gui_current_window)->name, arguments);
        }
        else
        {
            gui_printf (server->window,
                        _("%s \"kick\" command can only be executed in a channel window\n"),
                        WEECHAT_ERROR);
            return -1;
        }
    }
    return 0;
}

/*
 * irc_cmd_send_kill: close client-server connection
 */

int
irc_cmd_send_kill (t_irc_server *server, char *arguments)
{
    server_sendf (server, "KILL %s\r\n", arguments);
    return 0;
}

/*
 * irc_cmd_send_list: close client-server connection
 */

int
irc_cmd_send_list (t_irc_server *server, char *arguments)
{
    if (arguments)
        server_sendf (server, "LIST %s\r\n", arguments);
    else
        server_sendf (server, "LIST\r\n");
    return 0;
}

/*
 * irc_cmd_send_me: send a ctcp action to the current channel
 */

int
irc_cmd_send_me (t_irc_server *server, char *arguments)
{
    if (WIN_IS_SERVER(gui_current_window))
    {
        gui_printf (server->window,
                    _("%s \"me\" command can not be executed on a server window\n"),
                    WEECHAT_ERROR);
        return -1;
    }
    server_sendf (server, "PRIVMSG %s :\01ACTION %s\01\r\n",
                  CHANNEL(gui_current_window)->name, arguments);
    irc_display_prefix (gui_current_window, PREFIX_ACTION_ME);
    gui_printf_color (gui_current_window,
                      COLOR_WIN_CHAT_NICK, "%s", server->nick);
    gui_printf_color (gui_current_window,
                      COLOR_WIN_CHAT, " %s\n", arguments);
    return 0;
}

/*
 * irc_cmd_send_mode: change mode for channel/nickname
 */

int
irc_cmd_send_mode (t_irc_server *server, char *arguments)
{
    server_sendf (server, "MODE %s\r\n", arguments);
    return 0;
}

/*
 * irc_cmd_send_msg: send a message to a nick or channel
 */

int
irc_cmd_send_msg (t_irc_server *server, char *arguments)
{
    char *pos, *pos_comma;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
        
        while (arguments && arguments[0])
        {
            pos_comma = strchr (arguments, ',');
            if (pos_comma)
            {
                pos_comma[0] = '\0';
                pos_comma++;
            }
            if (string_is_channel (arguments))
            {
                ptr_channel = channel_search (server, arguments);
                if (ptr_channel)
                {
                    ptr_nick = nick_search (ptr_channel, server->nick);
                    if (ptr_nick)
                    {
                        irc_display_nick (ptr_channel->window, ptr_nick,
                                          MSG_TYPE_NICK, 1, 1, 0);
                        gui_printf_color_type (ptr_channel->window,
                                               MSG_TYPE_MSG,
                                               COLOR_WIN_CHAT, "%s\n", pos);
                    }
                    else
                        gui_printf (server->window,
                                    _("%s nick not found for \"privmsg\" command\n"),
                                    WEECHAT_ERROR);
                }
                server_sendf (server, "PRIVMSG %s :%s\r\n", arguments, pos);
            }
            else
            {
                ptr_channel = channel_search (server, arguments);
                if (!ptr_channel)
                {
                    ptr_channel = channel_new (server, CHAT_PRIVATE, arguments);
                    if (!ptr_channel)
                    {
                        gui_printf (server->window,
                                    _("%s cannot create new private window \"%s\"\n"),
                                    WEECHAT_ERROR,
                                    arguments);
                        return -1;
                    }
                    gui_redraw_window_title (ptr_channel->window);
                }
                    
                gui_printf_color_type (ptr_channel->window,
                                       MSG_TYPE_NICK,
                                       COLOR_WIN_CHAT_DARK, "<");
                gui_printf_color_type (ptr_channel->window,
                                       MSG_TYPE_NICK,
                                       COLOR_WIN_NICK_SELF,
                                       "%s", server->nick);
                gui_printf_color_type (ptr_channel->window,
                                       MSG_TYPE_NICK,
                                       COLOR_WIN_CHAT_DARK, "> ");
                gui_printf_color_type (ptr_channel->window,
                                       MSG_TYPE_MSG,
                                       COLOR_WIN_CHAT, "%s\n", pos);
                server_sendf (server, "PRIVMSG %s :%s\r\n", arguments, pos);
            }
            arguments = pos_comma;
        }
    }
    else
        gui_printf (server->window,
                    _("%s wrong number of args for \"privmsg\" command\n"),
                    WEECHAT_ERROR);
    return 0;
}

/*
 * irc_cmd_send_names: list nicknames on channels
 */

int
irc_cmd_send_names (t_irc_server *server, char *arguments)
{
    if (arguments)
        server_sendf (server, "NAMES %s\r\n", arguments);
    else
    {
        if (!WIN_IS_CHANNEL(gui_current_window))
        {
            gui_printf (server->window,
                        _("%s \"names\" command can only be executed in a channel window\n"),
                        WEECHAT_ERROR);
            return -1;
        }
        else
            server_sendf (server, "NAMES %s\r\n",
                          CHANNEL(gui_current_window)->name);
    }
    return 0;
}

/*
 * irc_cmd_send_nick: change nickname
 */

int
irc_cmd_send_nick (t_irc_server *server, int argc, char **argv)
{
    if (argc != 1)
        return -1;
    server_sendf (server, "NICK %s\r\n", argv[0]);
    return 0;
}

/*
 * irc_cmd_send_notice: send notice message
 */

int
irc_cmd_send_notice (t_irc_server *server, char *arguments)
{
    server_sendf (server, "NOTICE %s\r\n", arguments);
    return 0;
}

/*
 * irc_cmd_send_op: give operator privileges to nickname(s)
 */

int
irc_cmd_send_op (t_irc_server *server, int argc, char **argv)
{
    int i;
    
    if (WIN_IS_CHANNEL(gui_current_window))
    {
        for (i = 0; i < argc; i++)
            server_sendf (server, "MODE %s +o %s\r\n",
                          CHANNEL(gui_current_window)->name,
                          argv[i]);
    }
    else
    {
        gui_printf (server->window,
                    _("%s \"op\" command can only be executed in a channel window\n"),
                    WEECHAT_ERROR);
        return -1;
    }
    return 0;
}

/*
 * irc_cmd_send_oper: get oper privileges
 */

int
irc_cmd_send_oper (t_irc_server *server, int argc, char **argv)
{
    if (argc != 2)
        return -1;
    server_sendf (server, "OPER %s %s\r\n", argv[0], argv[1]);
    return 0;
}

/*
 * irc_cmd_send_part: leave a channel or close a private window
 */

int
irc_cmd_send_part (t_irc_server *server, char *arguments)
{
    char *channel_name, *pos_args;
    t_irc_channel *ptr_channel;
    
    if (arguments)
    {
        if (string_is_channel (arguments))
        {
            channel_name = arguments;
            pos_args = strchr (arguments, ' ');
            if (pos_args)
            {
                pos_args[0] = '\0';
                pos_args++;
                while (pos_args[0] == ' ')
                    pos_args++;
            }
        }
        else
        {
            if (WIN_IS_SERVER(gui_current_window))
            {
                gui_printf (server->window,
                            _("%s \"part\" command can not be executed on a server window\n"),
                            WEECHAT_ERROR);
                return -1;
            }
            channel_name = CHANNEL(gui_current_window)->name;
            pos_args = arguments;
        }
    }
    else
    {
        if (WIN_IS_SERVER(gui_current_window))
        {
            gui_printf (server->window,
                        _("%s \"part\" command can not be executed on a server window\n"),
                        WEECHAT_ERROR);
            return -1;
        }
        if (WIN_IS_PRIVATE(gui_current_window))
        {
            ptr_channel = CHANNEL(gui_current_window);
            gui_window_free (ptr_channel->window);
            channel_free (server, ptr_channel);
            gui_redraw_window_status (gui_current_window);
            gui_redraw_window_input (gui_current_window);
            return 0;
        }
        channel_name = CHANNEL(gui_current_window)->name;
        pos_args = NULL;
    }
    
    if (pos_args)
        server_sendf (server, "PART %s :%s\r\n", channel_name, pos_args);
    else
        server_sendf (server, "PART %s\r\n", channel_name);
    return 0;
}

/*
 * irc_cmd_send_ping: ping a server
 */

int
irc_cmd_send_ping (t_irc_server *server, int argc, char **argv)
{
    if (argc == 1)
        server_sendf (server, "PING %s\r\n", argv[0]);
    if (argc == 2)
        server_sendf (server, "PING %s %s\r\n", argv[0],
                      argv[1]);
    return 0;
}

/*
 * irc_cmd_send_pong: send pong answer to a daemon
 */

int
irc_cmd_send_pong (t_irc_server *server, int argc, char **argv)
{
    if (argc == 1)
        server_sendf (server, "PONG %s\r\n", argv[0]);
    if (argc == 2)
        server_sendf (server, "PONG %s %s\r\n", argv[0],
                      argv[1]);
    return 0;
}

/*
 * irc_cmd_send_quit: disconnect from all servers and quit WeeChat
 */

int
irc_cmd_send_quit (t_irc_server *server, char *arguments)
{
    if (server && server->is_connected)
    {
        if (arguments)
            server_sendf (server, "QUIT :%s\r\n", arguments);
        else
            server_sendf (server, "QUIT\r\n");
    }
    quit_weechat = 1;
    return 0;
}

/*
 * irc_cmd_send_quote: send raw data to server
 */

int
irc_cmd_send_quote (t_irc_server *server, char *arguments)
{
    server_sendf (server, "%s\r\n", arguments);
    return 0;
}

/*
 * irc_cmd_send_topic: get/set topic for a channel
 */

int
irc_cmd_send_topic (t_irc_server *server, char *arguments)
{
    char *channel_name, *new_topic, *pos;
    
    channel_name = NULL;
    new_topic = NULL;
    
    if (arguments)
    {
        if (string_is_channel (arguments))
        {
            channel_name = arguments;
            pos = strchr (arguments, ' ');
            if (pos)
            {
                pos[0] = '\0';
                pos++;
                while (pos[0] == ' ')
                    pos++;
                new_topic = (pos[0]) ? pos : NULL;
            }
        }
        else
            new_topic = arguments;
    }
    
    /* look for current channel if not specified */
    if (!channel_name)
    {
        if (WIN_IS_SERVER(gui_current_window))
        {
            gui_printf (server->window,
                        _("%s \"topic\" command can not be executed on a server window\n"),
                        WEECHAT_ERROR);
            return -1;
        }
        channel_name = CHANNEL(gui_current_window)->name;
    }
    
    if (new_topic)
    {
        if (strcmp (new_topic, "-delete") == 0)
            server_sendf (server, "TOPIC %s :\r\n", channel_name);
        else
            server_sendf (server, "TOPIC %s :%s\r\n", channel_name, new_topic);
    }
    else
        server_sendf (server, "TOPIC %s\r\n", channel_name);
    return 0;
}

/*
 * irc_cmd_send_version: gives the version info of nick or server (current or specified)
 */

int
irc_cmd_send_version (t_irc_server *server, char *arguments)
{
    if (arguments)
    {
        if (WIN_IS_CHANNEL(gui_current_window) &&
            nick_search (CHANNEL(gui_current_window), arguments))
            server_sendf (server, "PRIVMSG %s :\01VERSION\01\r\n",
                          arguments);
        else
            server_sendf (server, "VERSION %s\r\n",
                          arguments);
    }
    else
    {
        irc_display_prefix (server->window, PREFIX_INFO);
        gui_printf (server->window, "%s, compiled on %s %s\n",
                    WEECHAT_NAME_AND_VERSION,
                    __DATE__, __TIME__);
        server_sendf (server, "VERSION\r\n");
    }
    return 0;
}

/*
 * irc_cmd_send_voice: give voice to nickname(s)
 */

int
irc_cmd_send_voice (t_irc_server *server, int argc, char **argv)
{
    int i;
    
    if (WIN_IS_CHANNEL(gui_current_window))
    {
        for (i = 0; i < argc; i++)
            server_sendf (server, "MODE %s +v %s\r\n",
                          CHANNEL(gui_current_window)->name,
                          argv[i]);
    }
    else
    {
        gui_printf (server->window,
                    _("%s \"voice\" command can only be executed in a channel window\n"),
                    WEECHAT_ERROR);
        return -1;
    }
    return 0;
}

/*
 * irc_cmd_send_whois: query information about user(s)
 */

int
irc_cmd_send_whois (t_irc_server *server, char *arguments)
{
    server_sendf (server, "WHOIS %s\r\n", arguments);
    return 0;
}

/*
 * irc_cmd_recv_error: error received from server
 */

int
irc_cmd_recv_error (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos2;
    int first;
    
    /* make gcc happy */
    (void) server;
    (void) host;
    
    if (strncmp (arguments, "Closing Link", 12) == 0)
    {
        server_disconnect (server);
        return 0;
    }
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
    }
    else
        pos = arguments;
    
    irc_display_prefix (server->window, PREFIX_ERROR);
    first = 1;
    
    while (pos && pos[0])
    {
        pos2 = strchr (pos, ' ');
        if ((pos[0] == ':') || (!pos2))
        {
            if (pos[0] == ':')
                pos++;
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT,
                              "%s%s\n", (first) ? "" : ": ", pos);
            pos = NULL;
        }
        else
        {
            pos2[0] = '\0';
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_CHANNEL,
                              "%s%s",
                              (first) ? "" : " ", pos);
            first = 0;
            pos = pos2 + 1;
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_join: 'join' message received
 */

int
irc_cmd_recv_join (t_irc_server *server, char *host, char *arguments)
{
    t_irc_channel *ptr_channel;
    char *pos;
    
    ptr_channel = channel_search (server, arguments);
    if (!ptr_channel)
    {
        ptr_channel = channel_new (server, CHAT_CHANNEL, arguments);
        if (!ptr_channel)
        {
            gui_printf (server->window,
                        _("%s cannot create new channel \"%s\"\n"),
                        WEECHAT_ERROR, arguments);
            return -1;
        }
    }
    
    pos = strchr (host, '!');
    if (pos)
        pos[0] = '\0';
    
    irc_display_prefix (ptr_channel->window, PREFIX_JOIN);
    gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_NICK,
                      "%s ", host);
    gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_DARK,
                      "(");
    gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_HOST,
                      "%s", pos + 1);
    gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_DARK,
                      ")");
    gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT,
                      _(" has joined "));
    gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_CHANNEL,
                      "%s\n", arguments);
    nick_new (ptr_channel, host, 0, 0, 0);
    gui_redraw_window_nick (gui_current_window);
    return 0;
}

/*
 * irc_cmd_recv_kick: 'kick' message received
 */

int
irc_cmd_recv_kick (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos_nick, *pos_comment;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    
    pos = strchr (host, '!');
    if (pos)
        pos[0] = '\0';
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        pos_nick[0] = '\0';
        pos_nick++;
        while (pos_nick[0] == ' ')
            pos_nick++;
    
        pos_comment = strchr (pos_nick, ' ');
        if (pos_comment)
        {
            pos_comment[0] = '\0';
            pos_comment++;
            while (pos_comment[0] == ' ')
                pos_comment++;
            if (pos_comment[0] == ':')
                pos_comment++;
        }
    
        ptr_channel = channel_search (server, arguments);
        if (!ptr_channel)
        {
            gui_printf (server->window,
                        _("%s channel not found for \"kick\" command\n"),
                        WEECHAT_ERROR);
            return -1;
        }
    
        irc_display_prefix (ptr_channel->window, PREFIX_PART);
        gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_NICK,
                          "%s", host);
        gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT,
                          _(" has kicked "));
        gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_NICK,
                          "%s", pos_nick);
        gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT,
                          _(" from "));
        if (pos_comment)
        {
            gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_CHANNEL,
                              "%s ", arguments);
            gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_DARK,
                              "(");
            gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT,
                              "%s", pos_comment);
            gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_DARK,
                              ")\n");
        }
        else
            gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_CHANNEL,
                              "%s\n", arguments);
    }
    else
    {
        gui_printf (server->window,
                    _("%s nick not found for \"kick\" command\n"),
                    WEECHAT_ERROR);
        return -1;
    }
    ptr_nick = nick_search (ptr_channel, pos_nick);
    if (ptr_nick)
    {
        nick_free (ptr_channel, ptr_nick);
        gui_redraw_window_nick (gui_current_window);
    }
    return 0;
}

/*
 * irc_cmd_recv_mode: 'mode' message received
 */

int
irc_cmd_recv_mode (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos2, *pos_parm;
    char set_flag;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    
    /* no host => we can't identify sender of message! */
    if (host == NULL)
    {
        gui_printf (server->window,
                    _("%s \"mode\" command received without host\n"),
                    WEECHAT_ERROR);
        return -1;
    }
    
    /* keep only nick name from host */
    pos = strchr (host, '!');
    if (pos)
        pos[0] = '\0';
    
    pos = strchr (arguments, ' ');
    if (!pos)
    {
        gui_printf (server->window,
                    _("%s \"mode\" command received without channel or nickname\n"),
                    WEECHAT_ERROR);
        return -1;
    }
    pos[0] = '\0';
    pos++;
    while (pos[0] == ' ')
        pos++;
    
    pos_parm = strchr (pos, ' ');
    if (pos_parm)
    {
        pos_parm[0] = '\0';
        pos_parm++;
        while (pos_parm[0] == ' ')
            pos_parm++;
        pos2 = strchr (pos_parm, ' ');
        if (pos2)
            pos2[0] = '\0';
    }
    
    set_flag = '+';
    
    if (string_is_channel (arguments))
    {
        ptr_channel = channel_search (server, arguments);
        if (ptr_channel)
        {
            /* channel modes */
            while (pos && pos[0])
            {
                switch (pos[0])
                {
                    case '+':
                        set_flag = '+';
                        break;
                    case '-':
                        set_flag = '-';
                        break;
                    case 'b':
                        irc_display_mode (ptr_channel->window,
                                          arguments, set_flag, "b", host,
                                          (set_flag == '+') ?
                                              _("sets ban on") :
                                              _("removes ban on"),
                                          pos_parm);
                        /* TODO: change & redraw channel modes */
                        break;
                    case 'i':
                        irc_display_mode (ptr_channel->window,
                                          arguments, set_flag, "i", host,
                                          (set_flag == '+') ?
                                              _("sets invite-only channel flag") :
                                              _("removes invite-only channel flag"),
                                          NULL);
                        /* TODO: change & redraw channel modes */
                        break;
                    case 'l':
                        irc_display_mode (ptr_channel->window,
                                          arguments, set_flag, "l", host,
                                          (set_flag == '+') ?
                                              _("sets the user limit to") :
                                              _("removes user limit"),
                                          (set_flag == '+') ? pos_parm : NULL);
                        /* TODO: change & redraw channel modes */
                        break;
                    case 'm':
                        irc_display_mode (ptr_channel->window,
                                          arguments, set_flag, "m", host,
                                          (set_flag == '+') ?
                                              _("sets moderated channel flag") :
                                              _("removes moderated channel flag"),
                                          NULL);
                        /* TODO: change & redraw channel modes */
                        break;
                    case 'o':
                        irc_display_mode (ptr_channel->window,
                                          arguments, set_flag, "o", host,
                                          (set_flag == '+') ?
                                              _("gives channel operator status to") :
                                              _("removes channel operator status from"),
                                          pos_parm);
                        ptr_nick = nick_search (ptr_channel, pos_parm);
                        if (ptr_nick)
                        {
                            ptr_nick->is_op = (set_flag == '+') ? 1 : 0;
                            nick_resort (ptr_channel, ptr_nick);
                            gui_redraw_window_nick (ptr_channel->window);
                        }
                        break;
                    /* TODO: remove this obsolete (?) channel flag? */
                    case 'p':
                        irc_display_mode (ptr_channel->window,
                                          arguments, set_flag, "p", host,
                                          (set_flag == '+') ?
                                              _("sets private channel flag") :
                                              _("removes private channel flag"),
                                          NULL);
                        /* TODO: change & redraw channel modes */
                        break;
                    case 's':
                        irc_display_mode (ptr_channel->window,
                                          arguments, set_flag, "s", host,
                                          (set_flag == '+') ?
                                              _("sets secret channel flag") :
                                              _("removes secret channel flag"),
                                          NULL);
                        /* TODO: change & redraw channel modes */
                        break;
                    case 't':
                        irc_display_mode (ptr_channel->window,
                                          arguments, set_flag, "t", host,
                                          (set_flag == '+') ?
                                              _("sets topic protection") :
                                              _("removes topic protection"),
                                          NULL);
                        /* TODO: change & redraw channel modes */
                        break;
                    case 'v':
                        irc_display_mode (ptr_channel->window,
                                          arguments, set_flag, "v", host,
                                          (set_flag == '+') ?
                                              _("gives voice to") :
                                              _("removes voice from"),
                                          pos_parm);
                        
                        ptr_nick = nick_search (ptr_channel, pos_parm);
                        if (ptr_nick)
                        {
                            ptr_nick->has_voice = (set_flag == '+') ? 1 : 0;
                            nick_resort (ptr_channel, ptr_nick);
                            gui_redraw_window_nick (ptr_channel->window);
                        }
                        break;
                }
                pos++;
            }
        }
        else
        {
            gui_printf (server->window,
                        _("%s channel not found for \"mode\" command\n"),
                        WEECHAT_ERROR);
            return -1;
        }
    }
    else
    {
        /* nickname modes */
        gui_printf (server->window, "(TODO!) nickname modes: channel=%s, args=%s\n", arguments, pos);
    }
    return 0;
}

/*
 * irc_cmd_recv_nick: 'nick' message received
 */

int
irc_cmd_recv_nick (t_irc_server *server, char *host, char *arguments)
{
    char *pos;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    int nick_is_me;
    
    /* no host => we can't identify sender of message! */
    if (host == NULL)
    {
        gui_printf (server->window,
                    _("%s \"nick\" command received without host\n"),
                    WEECHAT_ERROR);
        return -1;
    }
    
    /* keep only nick name from host */
    pos = strchr (host, '!');
    if (pos)
        pos[0] = '\0';
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        ptr_nick = nick_search (ptr_channel, host);
        if (ptr_nick)
        {
            nick_is_me = (strcmp (ptr_nick->nick, server->nick) == 0);
            nick_change (ptr_channel, ptr_nick, arguments);
            irc_display_prefix (ptr_channel->window, PREFIX_INFO);
            if (nick_is_me)
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT,
                                  _("You are "));
            else
            {
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_NICK,
                                  "%s", host);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT, " is ");
            }
            gui_printf_color (ptr_channel->window,
                              COLOR_WIN_CHAT,
                              _("now known as "));
            gui_printf_color (ptr_channel->window,
                              COLOR_WIN_CHAT_NICK,
                              "%s\n",
                              arguments);
            if (ptr_channel->window->win_nick)
                gui_redraw_window_nick (ptr_channel->window);
        }
    }
    
    if (strcmp (server->nick, host) == 0)
    {
        free (server->nick);
        server->nick = strdup (arguments);
    }
    gui_redraw_window_input (gui_current_window);
    
    return 0;
}

/*
 * irc_cmd_recv_notice: 'notice' message received
 */

int
irc_cmd_recv_notice (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos2;
    
    if (host)
    {
        pos = strchr (host, '!');
        if (pos)
            pos[0] = '\0';
    }
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
        if (pos[0] == ':')
            pos++;
    }
    else
    {
        gui_printf (server->window,
                    _("%s nickname not found for \"notice\" command\n"),
                    WEECHAT_ERROR);
        return -1;
    }
    irc_display_prefix (server->window, PREFIX_SERVER);
    if (strncmp (pos, "\01VERSION", 8) == 0)
    {
        pos += 9;
        pos2 = strchr (pos, '\01');
        if (pos2)
            pos2[0] = '\0';
        gui_printf_color (server->window, COLOR_WIN_CHAT, "CTCP ");
        gui_printf_color (server->window, COLOR_WIN_CHAT_CHANNEL, "VERSION");
        gui_printf_color (server->window, COLOR_WIN_CHAT, " reply from ");
        gui_printf_color (server->window, COLOR_WIN_CHAT_NICK, "%s", host);
        gui_printf_color (server->window, COLOR_WIN_CHAT, ": %s\n", pos);
    }
    else
        gui_printf_color (server->window, COLOR_WIN_CHAT, "%s\n", pos);
    return 0;
}

/*
 * irc_cmd_recv_part: 'part' message received
 */

int
irc_cmd_recv_part (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos_args;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    
    /* no host => we can't identify sender of message! */
    if (!host || !arguments)
    {
        gui_printf (server->window,
                    _("%s \"part\" command received without host or channel\n"),
                    WEECHAT_ERROR);
        return -1;
    }
    
    pos_args = strchr (arguments, ' ');
    if (pos_args)
    {
        pos_args[0] = '\0';
        pos_args++;
        while (pos_args[0] == ' ')
            pos_args++;
        if (pos_args[0] == ':')
            pos_args++;
    }
    
    /* keep only nick name from host */
    pos = strchr (host, '!');
    if (pos)
        pos[0] = '\0';
    
    ptr_channel = channel_search (server, arguments);
    if (ptr_channel)
    {
        ptr_nick = nick_search (ptr_channel, host);
        if (ptr_nick)
        {
            if (strcmp (ptr_nick->nick, server->nick) == 0)
            {
                /* part request was issued by local client */
                gui_window_free (ptr_channel->window);
                channel_free (server, ptr_channel);
                gui_redraw_window_status (gui_current_window);
                gui_redraw_window_input (gui_current_window);
            }
            else
            {
            
                /* remove nick from nick list and display message */
                nick_free (ptr_channel, ptr_nick);
                irc_display_prefix (ptr_channel->window, PREFIX_PART);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_NICK, "%s ", host);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_DARK, "(");
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_HOST, "%s", pos+1);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_DARK, ")");
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT, _(" has left "));
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_CHANNEL,
                                  "%s", ptr_channel->name);
                if (pos_args && pos_args[0])
                {
                    gui_printf_color (ptr_channel->window,
                                      COLOR_WIN_CHAT_DARK, " (");
                    gui_printf_color (ptr_channel->window,
                                      COLOR_WIN_CHAT, "%s", pos_args);
                    gui_printf_color (ptr_channel->window,
                                      COLOR_WIN_CHAT_DARK, ")");
                }
                gui_printf (ptr_channel->window, "\n");
            
                /* redraw nick list if this is current window */
                if (ptr_channel->window->win_nick)
                    gui_redraw_window_nick (ptr_channel->window);
            }
        }
    }
    else
    {
        gui_printf (server->window,
                    _("%s channel not found for \"part\" command\n"),
                    WEECHAT_ERROR);
        return -1;
    }
    
    return 0;
}

/*
 * irc_cmd_recv_ping: 'ping' command received
 */

int
irc_cmd_recv_ping (t_irc_server *server, char *host, char *arguments)
{
    char *pos;
    
    (void)host;
    pos = strrchr (arguments, ' ');
    if (pos)
        pos[0] = '\0';
    server_sendf (server, "PONG :%s\r\n", arguments);
    return 0;
}

/*
 * irc_cmd_recv_privmsg: 'privmsg' command received
 */

int
irc_cmd_recv_privmsg (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos2, *host2;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    struct utsname *buf;
    
    /* no host => we can't identify sender of message! */
    if (host == NULL)
    {
        gui_printf (server->window,
                    _("%s \"privmsg\" command received without host\n"),
                    WEECHAT_ERROR);
        return -1;
    }
    
    /* keep only nick name from host */
    pos = strchr (host, '!');
    if (pos)
    {
        pos[0] = '\0';
        host2 = pos+1;
    }
    else
        host2 = host;
    
    /* receiver is a channel ? */
    if (string_is_channel (arguments))
    {
        pos = strchr (arguments, ' ');
        if (pos)
        {
            pos[0] = '\0';
            pos++;
            while (pos[0] == ' ')
                pos++;
            if (pos[0] == ':')
                pos++;
            
            ptr_channel = channel_search (server, arguments);
            if (ptr_channel)
            {
                if (strncmp (pos, "\01ACTION ", 8) == 0)
                {
                    pos += 8;
                    pos2 = strchr (pos, '\01');
                    if (pos2)
                        pos2[0] = '\0';
                    irc_display_prefix (ptr_channel->window, PREFIX_ACTION_ME);
                    gui_printf_color (ptr_channel->window,
                                      COLOR_WIN_CHAT_NICK, "%s", host);
                    gui_printf_color (ptr_channel->window,
                                      COLOR_WIN_CHAT, " %s\n", pos);
                }
                else
                {
                    ptr_nick = nick_search (ptr_channel, host);
                    if (ptr_nick)
                    {
                        irc_display_nick (ptr_channel->window, ptr_nick,
                                          MSG_TYPE_NICK, 1, 1, 0);
                        gui_printf_color_type (ptr_channel->window,
                                               MSG_TYPE_MSG,
                                               COLOR_WIN_CHAT, "%s\n", pos);
                    }
                    else
                    {
                        gui_printf (server->window,
                                    _("%s nick not found for \"privmsg\" command\n"),
                                    WEECHAT_ERROR);
                        return -1;
                    }
                }
            }
            else
            {
                gui_printf (server->window,
                            _("%s channel not found for \"privmsg\" command\n"),
                            WEECHAT_ERROR);
                return -1;
            }
        }
    }
    else
    {
        pos = strchr (host, '!');
        if (pos)
            pos[0] = '\0';
        
        pos = strchr (arguments, ' ');
        if (pos)
        {
            pos[0] = '\0';
            pos++;
            while (pos[0] == ' ')
                pos++;
            if (pos[0] == ':')
                pos++;
            
            if (strcmp (pos, "\01VERSION\01") == 0)
            {
                buf = (struct utsname *) malloc (sizeof (struct utsname));
                uname (buf);
                server_sendf (server,
                              "NOTICE %s :\01VERSION "
                              WEECHAT_NAME " v"
                              WEECHAT_VERSION " compiled on " __DATE__
                              ", \"%s\" running %s %s on a %s\01\r\n",
                              host, &buf->nodename, &buf->sysname,
                              &buf->release, &buf->machine);
                free (buf);
                irc_display_prefix (server->window, PREFIX_INFO);
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT, _("Received a "));
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT_CHANNEL, _("CTCP VERSION "));
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT, _("from"));
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT_NICK, " %s\n", host);
            }
            else
            {
                /* private message received */
                ptr_channel = channel_search (server, host);
                if (!ptr_channel)
                {
                    ptr_channel = channel_new (server, CHAT_PRIVATE, host);
                    if (!ptr_channel)
                    {
                        gui_printf (server->window,
                                    _("%s cannot create new private window \"%s\"\n"),
                                    WEECHAT_ERROR, host);
                        return -1;
                    }
                }
                if (!ptr_channel->topic)
                {
                    ptr_channel->topic = strdup (host2);
                    gui_redraw_window_title (ptr_channel->window);
                }
                
                gui_printf_color_type (ptr_channel->window,
                                       MSG_TYPE_NICK,
                                       COLOR_WIN_CHAT_DARK, "<");
                gui_printf_color_type (ptr_channel->window,
                                       MSG_TYPE_NICK,
                                       COLOR_WIN_NICK_PRIVATE,
                                       "%s", host);
                gui_printf_color_type (ptr_channel->window,
                                       MSG_TYPE_NICK,
                                       COLOR_WIN_CHAT_DARK, "> ");
                gui_printf_color_type (ptr_channel->window,
                                       MSG_TYPE_MSG,
                                       COLOR_WIN_CHAT, "%s\n", pos);
            }
        }
        else
        {
            gui_printf (server->window,
                        _("%s cannot parse \"privmsg\" command\n"),
                        WEECHAT_ERROR);
            return -1;
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_quit: 'quit' command received
 */

int
irc_cmd_recv_quit (t_irc_server *server, char *host, char *arguments)
{
    char *pos;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    
    /* no host => we can't identify sender of message! */
    if (host == NULL)
    {
        gui_printf (server->window,
                    _("%s \"quit\" command received without host\n"),
                    WEECHAT_ERROR);
        return -1;
    }
    
    /* keep only nick name from host */
    pos = strchr (host, '!');
    if (pos)
        pos[0] = '\0';
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (ptr_channel->type == CHAT_PRIVATE)
            ptr_nick = NULL;
        else
            ptr_nick = nick_search (ptr_channel, host);
        
        if (ptr_nick || (strcmp (ptr_channel->name, host) == 0))
        {
            if (ptr_nick)
                nick_free (ptr_channel, ptr_nick);
            irc_display_prefix (ptr_channel->window, PREFIX_QUIT);
            gui_printf_color (ptr_channel->window,
                              COLOR_WIN_CHAT_NICK, "%s ", host);
            gui_printf_color (ptr_channel->window,
                              COLOR_WIN_CHAT_DARK, "(");
            gui_printf_color (ptr_channel->window,
                              COLOR_WIN_CHAT_HOST, "%s", pos + 1);
            gui_printf_color (ptr_channel->window,
                              COLOR_WIN_CHAT_DARK, ")");
            gui_printf_color (ptr_channel->window,
                              COLOR_WIN_CHAT, _(" has quit "));
            gui_printf_color (ptr_channel->window,
                              COLOR_WIN_CHAT_DARK, "(");
            gui_printf_color (ptr_channel->window,
                              COLOR_WIN_CHAT, "%s",
                              arguments);
            gui_printf_color (ptr_channel->window,
                              COLOR_WIN_CHAT_DARK, ")\n");
            if ((ptr_channel->window == gui_current_window) &&
                (ptr_channel->window->win_nick))
                gui_redraw_window_nick (ptr_channel->window);
        }
    }
    
    return 0;
}

/*
 * irc_cmd_recv_server_msg: command received from server (numeric)
 */

int
irc_cmd_recv_server_msg (t_irc_server *server, char *host, char *arguments)
{
    /* make gcc happy */
    (void) host;
    
    /* skip nickname if at beginning of server message */
    if (strncmp (server->nick, arguments, strlen (server->nick)) == 0)
    {
        arguments += strlen (server->nick) + 1;
        while (arguments[0] == ' ')
            arguments++;
    }
    
    if (arguments[0] == ':')
        arguments++;
    
    /* display server message */
    irc_display_prefix (server->window, PREFIX_SERVER);
    gui_printf_color (server->window, COLOR_WIN_CHAT, "%s\n", arguments);
    return 0;
}

/*
 * irc_cmd_recv_server_reply: server reply
 */

int
irc_cmd_recv_server_reply (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos2;
    int first;
    
    /* make gcc happy */
    (void) server;
    (void) host;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
    }
    else
        pos = arguments;
    
    irc_display_prefix (server->window, PREFIX_ERROR);
    first = 1;
    
    while (pos && pos[0])
    {
        pos2 = strchr (pos, ' ');
        if ((pos[0] == ':') || (!pos2))
        {
            if (pos[0] == ':')
                pos++;
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT,
                              "%s%s\n", (first) ? "" : ": ", pos);
            pos = NULL;
        }
        else
        {
            pos2[0] = '\0';
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_CHANNEL,
                              "%s%s",
                              (first) ? "" : " ", pos);
            first = 0;
            pos = pos2 + 1;
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_topic: 'topic' command received
 */

int
irc_cmd_recv_topic (t_irc_server *server, char *host, char *arguments)
{
    char *pos;
    t_irc_channel *ptr_channel;
    t_gui_window *window;
    
    /* make gcc happy */
    (void) host;
    
    /* keep only nick name from host */
    pos = strchr (host, '!');
    if (pos)
        pos[0] = '\0';
    
    if (string_is_channel (arguments))
    {
        gui_printf (server->window,
                    _("%s \"topic\" command received without channel\n"),
                    WEECHAT_ERROR);
        return -1;
    }
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
        if (pos[0] == ':')
            pos++;
        if (!pos[0])
            pos = NULL;
    }
    
    ptr_channel = channel_search (server, arguments);
    window = (ptr_channel) ? ptr_channel->window : server->window;
    
    irc_display_prefix (window, PREFIX_INFO);
    gui_printf_color (window,
                      COLOR_WIN_CHAT_NICK, "%s",
                      host);
    if (pos)
    {
        gui_printf_color (window,
                          COLOR_WIN_CHAT, _(" has changed topic for "));
        gui_printf_color (window,
                          COLOR_WIN_CHAT_CHANNEL, "%s",
                          arguments);
        gui_printf_color (window,
                          COLOR_WIN_CHAT, _(" to: \"%s\"\n"),
                          pos);
    }
    else
    {
        gui_printf_color (window,
                          COLOR_WIN_CHAT, _(" has unset topic for "));
        gui_printf_color (window,
                          COLOR_WIN_CHAT_CHANNEL, "%s\n",
                          arguments);
    }
    
    if (ptr_channel)
    {
        if (ptr_channel->topic)
            free (ptr_channel->topic);
        if (pos)
            ptr_channel->topic = strdup (pos);
        else
            ptr_channel->topic = strdup ("");
        gui_redraw_window_title (ptr_channel->window);
    }
    
    return 0;
}

/*
 * irc_cmd_recv_004: '004' command (connected to irc server ?????)
 */

int
irc_cmd_recv_004 (t_irc_server *server, char *host, char *arguments)
{
    /* make gcc happy */
    (void) host;
    (void) arguments;
    
    irc_cmd_recv_server_msg (server, host, arguments);
    
    /* connection to IRC server is ok! */
    server->is_connected = 1;
    gui_redraw_window_status (server->window);
    gui_redraw_window_input (server->window);
    return 0;
}

/*
 * irc_cmd_recv_311: '311' command (away message)
 */

int
irc_cmd_recv_301 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_nick, *pos_message;
    
    /* make gcc happy */
    (void) server;
    (void) host;
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        while (pos_nick[0] == ' ')
            pos_nick++;
        pos_message = strchr (pos_nick, ' ');
        if (pos_message)
        {
            pos_message[0] = '\0';
            pos_message++;
            while (pos_message[0] == ' ')
                pos_message++;
            if (pos_message[0] == ':')
                pos_message++;
            
            irc_display_prefix (gui_current_window, PREFIX_INFO);
            gui_printf_color (gui_current_window,
                              COLOR_WIN_CHAT_NICK, "%s", pos_nick);
            gui_printf_color (gui_current_window,
                              COLOR_WIN_CHAT, _(" is away: %s\n"), pos_message);
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_311: '311' command (whois, user)
 */

int
irc_cmd_recv_311 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_nick, *pos_user, *pos_host, *pos_realname;
    
    /* make gcc happy */
    (void) host;
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        while (pos_nick[0] == ' ')
            pos_nick++;
        pos_user = strchr (pos_nick, ' ');
        if (pos_user)
        {
            pos_user[0] = '\0';
            pos_user++;
            while (pos_user[0] == ' ')
                pos_user++;
            pos_host = strchr (pos_user, ' ');
            if (pos_host)
            {
                pos_host[0] = '\0';
                pos_host++;
                while (pos_host[0] == ' ')
                    pos_host++;
                pos_realname = strchr (pos_host, ' ');
                if (pos_realname)
                {
                    pos_realname[0] = '\0';
                    pos_realname++;
                    while (pos_realname[0] == ' ')
                        pos_realname++;
                    if (pos_realname[0] == '*')
                        pos_realname++;
                    while (pos_realname[0] == ' ')
                        pos_realname++;
                    if (pos_realname[0] == ':')
                        pos_realname++;
                    
                    irc_display_prefix (server->window, PREFIX_INFO);
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_DARK, "[");
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_NICK, "%s", pos_nick);
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_DARK, "] (");
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_HOST, "%s@%s",
                                      pos_user, pos_host);
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_DARK, ")");
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT, ": %s\n", pos_realname);
                }
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_312: '312' command (whois, server)
 */

int
irc_cmd_recv_312 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_nick, *pos_server, *pos_serverinfo;
    
    /* make gcc happy */
    (void) host;
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        while (pos_nick[0] == ' ')
            pos_nick++;
        pos_server = strchr (pos_nick, ' ');
        if (pos_server)
        {
            pos_server[0] = '\0';
            pos_server++;
            while (pos_server[0] == ' ')
                pos_server++;
            pos_serverinfo = strchr (pos_server, ' ');
            if (pos_serverinfo)
            {
                pos_serverinfo[0] = '\0';
                pos_serverinfo++;
                while (pos_serverinfo[0] == ' ')
                    pos_serverinfo++;
                if (pos_serverinfo[0] == ':')
                        pos_serverinfo++;
                
                irc_display_prefix (server->window, PREFIX_INFO);
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT_DARK, "[");
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT_NICK, "%s", pos_nick);
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT_DARK, "] ");
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT, "%s ", pos_server);
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT_DARK, "(");
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT, "%s", pos_serverinfo);
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT_DARK, ")\n");
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_313: '313' command (whois, operator)
 */

int
irc_cmd_recv_313 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_nick, *pos_message;
    
    /* make gcc happy */
    (void) host;
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        while (pos_nick[0] == ' ')
            pos_nick++;
        pos_message = strchr (pos_nick, ' ');
        if (pos_message)
        {
            pos_message[0] = '\0';
            pos_message++;
            while (pos_message[0] == ' ')
                pos_message++;
            if (pos_message[0] == ':')
                pos_message++;
            
            irc_display_prefix (server->window, PREFIX_INFO);
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_DARK, "[");
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_NICK, "%s", pos_nick);
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_DARK, "] ");
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT, "%s\n", pos_message);
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_317: '317' command (whois, idle)
 */

int
irc_cmd_recv_317 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_nick, *pos_idle, *pos_signon, *pos_message;
    int idle_time, day, hour, min, sec;
    time_t datetime;
    
    /* make gcc happy */
    (void) host;
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        while (pos_nick[0] == ' ')
            pos_nick++;
        pos_idle = strchr (pos_nick, ' ');
        if (pos_idle)
        {
            pos_idle[0] = '\0';
            pos_idle++;
            while (pos_idle[0] == ' ')
                pos_idle++;
            pos_signon = strchr (pos_idle, ' ');
            if (pos_signon)
            {
                pos_signon[0] = '\0';
                pos_signon++;
                while (pos_signon[0] == ' ')
                    pos_signon++;
                pos_message = strchr (pos_signon, ' ');
                if (pos_message)
                {
                    pos_message[0] = '\0';
                    
                    idle_time = atoi (pos_idle);
                    day = idle_time / (60 * 60 * 24);
                    hour = (idle_time % (60 * 60 * 24)) / (60 * 60);
                    min = ((idle_time % (60 * 60 * 24)) % (60 * 60)) / 60;
                    sec = ((idle_time % (60 * 60 * 24)) % (60 * 60)) % 60;
                    
                    irc_display_prefix (server->window, PREFIX_INFO);
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_DARK, "[");
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_NICK, "%s", pos_nick);
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_DARK, "] ");
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT, _("idle: "));
                    if (day > 0)
                    {
                        gui_printf_color (server->window,
                                          COLOR_WIN_CHAT_CHANNEL,
                                          "%d ", day);
                        gui_printf_color (server->window,
                                          COLOR_WIN_CHAT,
                                          (day > 1) ? _("days") : _("day"));
                        gui_printf_color (server->window,
                                          COLOR_WIN_CHAT,
                                          ", ");
                    }
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_CHANNEL,
                                      "%02d ", hour);
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT,
                                      (hour > 1) ? _("hours") : _("hour"));
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_CHANNEL,
                                      " %02d ", min);
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT,
                                      (min > 1) ? _("minutes") : _("minute"));
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_CHANNEL,
                                      " %02d ", sec);
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT,
                                      (sec > 1) ? _("seconds") : _("second"));
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT,
                                      ", ");
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT, _("signon at: "));
                    datetime = (time_t)(atol (pos_signon));
                    gui_printf_color (server->window,
                                      COLOR_WIN_CHAT_CHANNEL,
                                      "%s", ctime (&datetime));
                }
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_318: '318' command (whois, end)
 */

int
irc_cmd_recv_318 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_nick, *pos_message;
    
    /* make gcc happy */
    (void) host;
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        while (pos_nick[0] == ' ')
            pos_nick++;
        pos_message = strchr (pos_nick, ' ');
        if (pos_message)
        {
            pos_message[0] = '\0';
            pos_message++;
            while (pos_message[0] == ' ')
                pos_message++;
            if (pos_message[0] == ':')
                pos_message++;
            
            irc_display_prefix (server->window, PREFIX_INFO);
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_DARK, "[");
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_NICK, "%s", pos_nick);
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_DARK, "] ");
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT, "%s\n", pos_message);
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_319: '319' command (whois, end)
 */

int
irc_cmd_recv_319 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_nick, *pos_channel, *pos;
    
    /* make gcc happy */
    (void) host;
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        while (pos_nick[0] == ' ')
            pos_nick++;
        pos_channel = strchr (pos_nick, ' ');
        if (pos_channel)
        {
            pos_channel[0] = '\0';
            pos_channel++;
            while (pos_channel[0] == ' ')
                pos_channel++;
            if (pos_channel[0] == ':')
                pos_channel++;
            
            irc_display_prefix (server->window, PREFIX_INFO);
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_DARK, "[");
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_NICK, "%s", pos_nick);
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_DARK, "] ");
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT, _("Channels: "));
            
            while (pos_channel && pos_channel[0])
            {
                if (pos_channel[0] == '@')
                {
                    gui_printf_color (server->window,
                                      COLOR_WIN_NICK_OP, "@");
                    pos_channel++;
                }
                else
                {
                    if (pos_channel[0] == '%')
                    {
                        gui_printf_color (server->window,
                                          COLOR_WIN_NICK_HALFOP, "%");
                        pos_channel++;
                    }
                    else
                        if (pos_channel[0] == '+')
                        {
                            gui_printf_color (server->window,
                                              COLOR_WIN_NICK_VOICE, "+");
                            pos_channel++;
                        }
                }
                pos = strchr (pos_channel, ' ');
                if (pos)
                {
                    pos[0] = '\0';
                    pos++;
                    while (pos[0] == ' ')
                        pos++;
                }
                gui_printf_color (server->window,
                                  COLOR_WIN_CHAT_CHANNEL,
                                  "%s%s",
                                  pos_channel,
                                  (pos && pos[0]) ? " " : "\n");
                pos_channel = pos;
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_320: '320' command (whois, identified user)
 */

int
irc_cmd_recv_320 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_nick, *pos_message;
    
    /* make gcc happy */
    (void) host;
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        while (pos_nick[0] == ' ')
            pos_nick++;
        pos_message = strchr (pos_nick, ' ');
        if (pos_message)
        {
            pos_message[0] = '\0';
            pos_message++;
            while (pos_message[0] == ' ')
                pos_message++;
            if (pos_message[0] == ':')
                pos_message++;
            
            irc_display_prefix (server->window, PREFIX_INFO);
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_DARK, "[");
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_NICK, "%s", pos_nick);
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT_DARK, "] ");
            gui_printf_color (server->window,
                              COLOR_WIN_CHAT, "%s\n", pos_message);
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_321: '321' command (/list start)
 */

int
irc_cmd_recv_321 (t_irc_server *server, char *host, char *arguments)
{
    char *pos;
    
    /* make gcc happy */
    (void) host;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
    }
    else
        pos = arguments;
        
    irc_display_prefix (server->window, PREFIX_INFO);
    gui_printf (server->window, "%s\n", pos);
    return 0;
}

/*
 * irc_cmd_recv_322: '322' command (channel for /list)
 */

int
irc_cmd_recv_322 (t_irc_server *server, char *host, char *arguments)
{
    char *pos;
    
    /* make gcc happy */
    (void) host;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
    }
    else
        pos = arguments;
        
    irc_display_prefix (server->window, PREFIX_INFO);
    gui_printf (server->window, "%s\n", pos);
    return 0;
}

/*
 * irc_cmd_recv_323: '323' command (/list end)
 */

int
irc_cmd_recv_323 (t_irc_server *server, char *host, char *arguments)
{
    char *pos;
    
    /* make gcc happy */
    (void) host;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        while (pos[0] == ' ')
            pos++;
    }
    else
        pos = arguments;
        
    irc_display_prefix (server->window, PREFIX_INFO);
    gui_printf (server->window, "%s\n", pos);
    return 0;
}

/*
 * irc_cmd_recv_331: '331' command received (no topic for channel)
 */

int
irc_cmd_recv_331 (t_irc_server *server, char *host, char *arguments)
{
    char *pos;
    
    /* make gcc happy */
    (void) server;
    (void) host;
    
    pos = strchr (arguments, ' ');
    if (pos)
        pos[0] = '\0';
    gui_printf_color (gui_current_window,
                      COLOR_WIN_CHAT, _("No topic set for "));
    gui_printf_color (gui_current_window,
                      COLOR_WIN_CHAT_CHANNEL, "%s\n", arguments);
    return 0;
}

/*
 * irc_cmd_recv_332: '332' command received (topic of channel)
 */

int
irc_cmd_recv_332 (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos2;
    t_irc_channel *ptr_channel;
    
    /* make gcc happy */
    (void) host;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        while (pos[0] == ' ')
            pos++;
        pos2 = strchr (pos, ' ');
        if (pos2)
        {
            pos2[0] = '\0';
            ptr_channel = channel_search (server, pos);
            if (ptr_channel)
            {
                pos2++;
                while (pos2[0] == ' ')
                    pos2++;
                if (pos2[0] == ':')
                    pos2++;
                if (ptr_channel->topic)
                    free (ptr_channel->topic);
                ptr_channel->topic = strdup (pos2);
                
                irc_display_prefix (ptr_channel->window, PREFIX_INFO);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT, _("Topic for "));
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_CHANNEL, "%s", pos);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT, _(" is: \"%s\"\n"), pos2);
                
                gui_redraw_window_title (ptr_channel->window);
            }
            else
            {
                gui_printf (server->window,
                            _("%s channel not found for \"332\" command\n"),
                            WEECHAT_ERROR);
                return -1;
            }
        }
    }
    else
    {
        gui_printf (server->window,
                    _("%s cannot identify channel for \"332\" command\n"),
                    WEECHAT_ERROR);
        return -1;
    }
    return 0;
}

/*
 * irc_cmd_recv_333: '333' command received (infos about topic (nick / date)
 */

int
irc_cmd_recv_333 (t_irc_server *server, char *host, char *arguments)
{
    char *pos_channel, *pos_nick, *pos_date;
    t_irc_channel *ptr_channel;
    time_t datetime;
    
    /* make gcc happy */
    (void) host;
    
    pos_channel = strchr (arguments, ' ');
    if (pos_channel)
    {
        while (pos_channel[0] == ' ')
            pos_channel++;
        pos_nick = strchr (pos_channel, ' ');
        if (pos_nick)
        {
            pos_nick[0] = '\0';
            pos_nick++;
            while (pos_nick[0] == ' ')
                pos_nick++;
            pos_date = strchr (pos_nick, ' ');
            if (pos_date)
            {
                pos_date[0] = '\0';
                pos_date++;
                while (pos_date[0] == ' ')
                    pos_date++;
                
                ptr_channel = channel_search (server, pos_channel);
                if (ptr_channel)
                {
                    irc_display_prefix (ptr_channel->window, PREFIX_INFO);
                    gui_printf_color (ptr_channel->window,
                                      COLOR_WIN_CHAT, _("Topic set by "));
                    gui_printf_color (ptr_channel->window,
                                      COLOR_WIN_CHAT_NICK, "%s", pos_nick);
                    datetime = (time_t)(atol (pos_date));
                    gui_printf_color (ptr_channel->window,
                                      COLOR_WIN_CHAT, ", %s", ctime (&datetime));
                }
                else
                {
                    gui_printf (server->window,
                                _("%s channel not found for \"333\" command\n"),
                                WEECHAT_ERROR);
                    return -1;
                }
            }
            else
            {
                gui_printf (server->window,
                            _("%s cannot identify date/time for \"333\" command\n"),
                            WEECHAT_ERROR);
                return -1;
            }
        }
        else
        {
            gui_printf (server->window,
                        _("%s cannot identify nickname for \"333\" command\n"),
                        WEECHAT_ERROR);
            return -1;
        }
    }
    else
    {
        gui_printf (server->window,
                    _("%s cannot identify channel for \"333\" command\n"),
                    WEECHAT_ERROR);
        return -1;
    }
    return 0;
}

/*
 * irc_cmd_recv_351: '351' command received (server version)
 */

int
irc_cmd_recv_351 (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos2;
    
    /* make gcc happy */
    (void) server;
    (void) host;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
    }
    else
        pos = arguments;
    
    pos2 = strstr (pos, " :");
    if (pos2)
    {
        pos2[0] = '\0';
        pos2 += 2;
    }
    
    irc_display_prefix (server->window, PREFIX_SERVER);
    if (pos2)
        gui_printf (server->window, "%s %s\n", pos, pos2);
    else
        gui_printf (server->window, "%s\n", pos);
    return 0;
}

/*
 * irc_cmd_recv_353: '353' command received (list of users on a channel)
 */

int
irc_cmd_recv_353 (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos_nick;
    int is_op, is_halfop, has_voice;
    t_irc_channel *ptr_channel;
    
    /* make gcc happy */
    (void) host;
        
    pos = strstr (arguments, " = ");
    if (pos)
        arguments = pos + 3;
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        
        ptr_channel = channel_search (server, arguments);
        if (!ptr_channel)
            return 0;
        
        pos++;
        while (pos[0] == ' ')
            pos++;
        if (pos[0] != ':')
        {
            gui_printf (server->window,
                        _("%s cannot parse \"353\" command\n"),
                        WEECHAT_ERROR);
            return -1;
        }
        pos++;
        if (pos[0])
        {
            while (pos && pos[0])
            {
                is_op = 0;
                is_halfop = 0;
                has_voice = 0;
                while ((pos[0] == '@') || (pos[0] == '%') || (pos[0] == '+'))
                {
                    if (pos[0] == '@')
                        is_op = 1;
                    if (pos[0] == '%')
                        is_halfop = 1;
                    if (pos[0] == '+')
                        has_voice = 1;
                    pos++;
                }
                pos_nick = pos;
                pos = strchr (pos, ' ');
                if (pos)
                {
                    pos[0] = '\0';
                    pos++;
                }
                if (!nick_new (ptr_channel, pos_nick, is_op, is_halfop, has_voice))
                    gui_printf (server->window,
                                _("%s cannot create nick \"%s\" for channel \"%s\"\n"),
                                WEECHAT_ERROR, pos_nick, ptr_channel->name);
            }
        }
        gui_redraw_window_nick (ptr_channel->window);
    }
    else
    {
        gui_printf (server->window,
                    _("%s cannot parse \"353\" command\n"),
                    WEECHAT_ERROR);
        return -1;
    }
    return 0;
}

/*
 * irc_cmd_recv_366: '366' command received (end of /names list)
 */

int
irc_cmd_recv_366 (t_irc_server *server, char *host, char *arguments)
{
    char *pos, *pos2;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    int num_nicks, num_op, num_halfop, num_voice, num_normal;
    
    /* make gcc happy */
    (void) host;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        while (pos[0] == ' ')
            pos++;
        pos2 = strchr (pos, ' ');
        if (pos2)
        {
            pos2[0] = '\0';
            pos2++;
            while (pos2[0] == ' ')
                pos2++;
            if (pos2[0] == ':')
                pos2++;
            
            ptr_channel = channel_search (server, pos);
            if (ptr_channel)
            {
    
                /* display users on channel */
                irc_display_prefix (ptr_channel->window, PREFIX_INFO);
                gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT,
                                  _("Nicks "));
                gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_CHANNEL,
                                  "%s", ptr_channel->name);
                gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT, ": ");
                gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_DARK, "[");
                
                for (ptr_nick = ptr_channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
                {
                    irc_display_nick (ptr_channel->window, ptr_nick,
                                      MSG_TYPE_INFO, 0, 0, 1);
                    if (ptr_nick != ptr_channel->last_nick)
                        gui_printf (ptr_channel->window, " ");
                }
                gui_printf_color (ptr_channel->window, COLOR_WIN_CHAT_DARK, "]\n");
    
                /* display number of nicks, ops, halfops & voices on the channel */
                nick_count (ptr_channel, &num_nicks, &num_op, &num_halfop, &num_voice,
                            &num_normal);
                irc_display_prefix (ptr_channel->window, PREFIX_INFO);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT, _("Channel "));
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_CHANNEL,
                                  "%s", ptr_channel->name);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT, ": ");
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_CHANNEL,
                                  "%d ", num_nicks);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT,
                                  (num_nicks > 1) ? _("nicks") : _("nick"));
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_DARK, " (");
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_CHANNEL,
                                  "%d ", num_op);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT,
                                  (num_op > 1) ? _("ops") : _("op"));
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT,
                                  ", ");
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_CHANNEL,
                                  "%d ", num_halfop);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT,
                                  (num_halfop > 1) ? _("halfops") : _("halfop"));
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT,
                                  ", ");
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_CHANNEL,
                                  "%d ", num_voice);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT,
                                  (num_voice > 1) ? _("voices") : _("voice"));
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT,
                                  ", ");
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_CHANNEL,
                                  "%d ", num_normal);
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT,
                                  _("normal"));
                gui_printf_color (ptr_channel->window,
                                  COLOR_WIN_CHAT_DARK, ")\n");
            }
            else
            {
                irc_display_prefix (gui_current_window, PREFIX_INFO);
                gui_printf_color (gui_current_window,
                                  COLOR_WIN_CHAT_CHANNEL, pos);
                gui_printf_color (gui_current_window,
                                  COLOR_WIN_CHAT, ": %s\n", pos2);
                return 0;
            }
        }
    }
    return 0;
}

/*
 * irc_cmd_recv_433: '433' command received (nickname already in use)
 */

int
irc_cmd_recv_433 (t_irc_server *server, char *host, char *arguments)
{
    char hostname[128];
        
    if (!server->is_connected)
    {
        if (strcmp (server->nick, server->nick1) == 0)
        {
            gui_printf (server->window,
                        _(WEECHAT_NAME
                        ": nickname \"%s\" is already in use, "
                        "trying 2nd nickname \"%s\"\n"),
                        server->nick, server->nick2);
            free (server->nick);
            server->nick = strdup (server->nick2);
        }
        else
        {
            if (strcmp (server->nick, server->nick2) == 0)
            {
                gui_printf (server->window,
                            _(WEECHAT_NAME
                            ": nickname \"%s\" is already in use, "
                            "trying 3rd nickname \"%s\"\n"),
                            server->nick, server->nick3);
                free (server->nick);
                server->nick = strdup (server->nick3);
            }
            else
            {
                gui_printf (server->window,
                            _(WEECHAT_NAME
                            ": all declared nicknames are already in use, "
                            "closing connection with server!\n"));
                server_disconnect (server);
                return 0;
            }
        }
    
        gethostname (hostname, sizeof (hostname) - 1);
        hostname[sizeof (hostname) - 1] = '\0';
        if (!hostname[0])
            strcpy (hostname, _("unknown"));
        server_sendf (server,
                      "NICK %s\r\n",
                      server->nick);
    }
    else
        return irc_cmd_recv_error (server, host, arguments);
    return 0;
}
