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

/* irc-protocol.c: description of IRC commands, according to
                   RFC 1459,2810,2811,2812 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <sys/utsname.h>

#include "../../core/weechat.h"
#include "irc.h"
#include "../../core/alias.h"
#include "../../core/command.h"
#include "../../core/utf8.h"
#include "../../core/util.h"
#include "../../core/weechat-config.h"


t_irc_protocol_msg irc_protocol_messages[] =
{ { "error", N_("error received from IRC server"), irc_protocol_cmd_error },
  { "invite", N_("invite a nick on a channel"), irc_protocol_cmd_invite },
  { "join", N_("join a channel"), irc_protocol_cmd_join },
  { "kick", N_("forcibly remove a user from a channel"), irc_protocol_cmd_kick },
  { "kill", N_("close client-server connection"), irc_protocol_cmd_kill },
  { "mode", N_("change channel or user mode"), irc_protocol_cmd_mode },
  { "nick", N_("change current nickname"), irc_protocol_cmd_nick },
  { "notice", N_("send notice message to user"), irc_protocol_cmd_notice },
  { "part", N_("leave a channel"), irc_protocol_cmd_part },
  { "ping", N_("ping server"), irc_protocol_cmd_ping },
  { "pong", N_("answer to a ping message"), irc_protocol_cmd_pong },
  { "privmsg", N_("message received"), irc_protocol_cmd_privmsg },
  { "quit", N_("close all connections and quit"), irc_protocol_cmd_quit },
  { "topic", N_("get/set channel topic"), irc_protocol_cmd_topic },
  { "wallops", N_("send a message to all currently connected users who have "
    "set the 'w' user mode for themselves"), irc_protocol_cmd_wallops },
  { "001", N_("a server message"), irc_protocol_cmd_001 },
  { "005", N_("a server message"), irc_protocol_cmd_005 },
  { "221", N_("user mode string"), irc_protocol_cmd_221 },
  { "301", N_("away message"), irc_protocol_cmd_301 },
  { "302", N_("userhost"), irc_protocol_cmd_302 },
  { "303", N_("ison"), irc_protocol_cmd_303 },
  { "305", N_("unaway"), irc_protocol_cmd_305 },
  { "306", N_("now away"), irc_protocol_cmd_306 },
  { "307", N_("whois (registered nick)"), irc_protocol_cmd_whois_nick_msg },
  { "310", N_("whois (help mode)"), irc_protocol_cmd_310 },
  { "311", N_("whois (user)"), irc_protocol_cmd_311 },
  { "312", N_("whois (server)"), irc_protocol_cmd_312 },
  { "313", N_("whois (operator)"), irc_protocol_cmd_whois_nick_msg },
  { "314", N_("whowas"), irc_protocol_cmd_314 },
  { "315", N_("end of /who list"), irc_protocol_cmd_315 },
  { "317", N_("whois (idle)"), irc_protocol_cmd_317 },
  { "318", N_("whois (end)"), irc_protocol_cmd_whois_nick_msg },
  { "319", N_("whois (channels)"), irc_protocol_cmd_319 },
  { "320", N_("whois (identified user)"), irc_protocol_cmd_whois_nick_msg },
  { "321", N_("/list start"), irc_protocol_cmd_321 },  
  { "322", N_("channel (for /list)"), irc_protocol_cmd_322 },
  { "323", N_("/list end"), irc_protocol_cmd_323 },
  { "324", N_("channel mode"), irc_protocol_cmd_324 },
  { "326", N_("whois (has oper privs)"), irc_protocol_cmd_whois_nick_msg },
  { "327", N_("whois (host)"), irc_protocol_cmd_327 },
  { "329", N_("channel creation date"), irc_protocol_cmd_329 },
  { "331", N_("no topic for channel"), irc_protocol_cmd_331 },
  { "332", N_("topic of channel"), irc_protocol_cmd_332 },
  { "333", N_("infos about topic (nick and date changed)"), irc_protocol_cmd_333 },
  { "338", N_("whois (host)"), irc_protocol_cmd_338 },
  { "341", N_("inviting"), irc_protocol_cmd_341 },
  { "344", N_("channel reop"), irc_protocol_cmd_344 },
  { "345", N_("end of channel reop list"), irc_protocol_cmd_345 },
  { "348", N_("channel exception list"), irc_protocol_cmd_348 },
  { "349", N_("end of channel exception list"), irc_protocol_cmd_349 },
  { "351", N_("server version"), irc_protocol_cmd_351 },
  { "352", N_("who"), irc_protocol_cmd_352 },
  { "353", N_("list of nicks on channel"), irc_protocol_cmd_353 },
  { "366", N_("end of /names list"), irc_protocol_cmd_366 },
  { "367", N_("banlist"), irc_protocol_cmd_367 },
  { "368", N_("end of banlist"), irc_protocol_cmd_368 },
  { "378", N_("whois (connecting from)"), irc_protocol_cmd_whois_nick_msg },
  { "379", N_("whois (using modes)"), irc_protocol_cmd_whois_nick_msg },
  { "401", N_("no such nick/channel"), irc_protocol_cmd_error },
  { "402", N_("no such server"), irc_protocol_cmd_error },
  { "403", N_("no such channel"), irc_protocol_cmd_error },
  { "404", N_("cannot send to channel"), irc_protocol_cmd_error },
  { "405", N_("too many channels"), irc_protocol_cmd_error },
  { "406", N_("was no such nick"), irc_protocol_cmd_error },
  { "407", N_("was no such nick"), irc_protocol_cmd_error },
  { "409", N_("no origin"), irc_protocol_cmd_error },
  { "410", N_("no services"), irc_protocol_cmd_error },
  { "411", N_("no recipient"), irc_protocol_cmd_error },
  { "412", N_("no text to send"), irc_protocol_cmd_error },
  { "413", N_("no toplevel"), irc_protocol_cmd_error },
  { "414", N_("wilcard in toplevel domain"), irc_protocol_cmd_error },
  { "421", N_("unknown command"), irc_protocol_cmd_error },
  { "422", N_("MOTD is missing"), irc_protocol_cmd_error },
  { "423", N_("no administrative info"), irc_protocol_cmd_error },
  { "424", N_("file error"), irc_protocol_cmd_error },
  { "431", N_("no nickname given"), irc_protocol_cmd_error },
  { "432", N_("erroneous nickname"), irc_protocol_cmd_432 },
  { "433", N_("nickname already in use"), irc_protocol_cmd_433 },
  { "436", N_("nickname collision"), irc_protocol_cmd_error },
  { "437", N_("resource unavailable"), irc_protocol_cmd_error },
  { "438", N_("not authorized to change nickname"), irc_protocol_cmd_438 },
  { "441", N_("user not in channel"), irc_protocol_cmd_error },
  { "442", N_("not on channel"), irc_protocol_cmd_error },
  { "443", N_("user already on channel"), irc_protocol_cmd_error },
  { "444", N_("user not logged in"), irc_protocol_cmd_error },
  { "445", N_("summon has been disabled"), irc_protocol_cmd_error },
  { "446", N_("users has been disabled"), irc_protocol_cmd_error },
  { "451", N_("you are not registered"), irc_protocol_cmd_error },
  { "461", N_("not enough parameters"), irc_protocol_cmd_error },
  { "462", N_("you may not register"), irc_protocol_cmd_error },
  { "463", N_("your host isn't among the privileged"), irc_protocol_cmd_error },
  { "464", N_("password incorrect"), irc_protocol_cmd_error },
  { "465", N_("you are banned from this server"), irc_protocol_cmd_error },
  { "467", N_("channel key already set"), irc_protocol_cmd_error },
  { "470", N_("forwarding to another channel"), irc_protocol_cmd_error },
  { "471", N_("channel is already full"), irc_protocol_cmd_error },
  { "472", N_("unknown mode char to me"), irc_protocol_cmd_error },
  { "473", N_("cannot join channel (invite only)"), irc_protocol_cmd_error },
  { "474", N_("cannot join channel (banned from channel)"), irc_protocol_cmd_error },
  { "475", N_("cannot join channel (bad channel key)"), irc_protocol_cmd_error },
  { "476", N_("bad channel mask"), irc_protocol_cmd_error },
  { "477", N_("channel doesn't support modes"), irc_protocol_cmd_error },
  { "481", N_("you're not an IRC operator"), irc_protocol_cmd_error },
  { "482", N_("you're not channel operator"), irc_protocol_cmd_error },
  { "483", N_("you can't kill a server!"), irc_protocol_cmd_error },
  { "484", N_("your connection is restricted!"), irc_protocol_cmd_error },
  { "485", N_("user is immune from kick/deop"), irc_protocol_cmd_error },
  { "487", N_("network split"), irc_protocol_cmd_error },
  { "491", N_("no O-lines for your host"), irc_protocol_cmd_error },
  { "501", N_("unknown mode flag"), irc_protocol_cmd_error },
  { "502", N_("can't change mode for other users"), irc_protocol_cmd_error },
  { "671", N_("whois (secure connection)"), irc_protocol_cmd_671 },
  { "973", N_("whois (secure connection)"), irc_protocol_cmd_server_mode_reason },
  { "974", N_("whois (secure connection)"), irc_protocol_cmd_server_mode_reason },
  { "975", N_("whois (secure connection)"), irc_protocol_cmd_server_mode_reason },
  { NULL, NULL, NULL }
};


char *irc_message = NULL;


/*
 * irc_protocol_is_word_char: return 1 if given character is a "word character"
 */

int
irc_protocol_is_word_char (char *str)
{
    wint_t c = utf8_get_wide_char (str);

    if (c == WEOF)
        return 0;
    
    if (iswalnum (c))
        return 1;
    
    switch (c)
    {
        case '-':
        case '_':
        case '|':
            return 1;
    }
    
    /* not a 'word char' */
    return 0;
}

/*
 * irc_protocol_is_numeric_command: return 1 if given string is 100% numeric
 */

int
irc_protocol_is_numeric_command (char *str)
{
    while (str && str[0])
    {
        if (!isdigit (str[0]))
            return 0;
        str++;
    }
    return 1;
}

/*
 * irc_protocol_is_highlight: return 1 if given message contains highlight (with given nick
 *                            or at least one of string in "irc_higlight" setting)
 */

int
irc_protocol_is_highlight (char *message, char *nick)
{
    char *msg, *highlight, *match, *match_pre, *match_post, *msg_pos, *pos, *pos_end;
    int end, length, startswith, endswith, wildcard_start, wildcard_end;
    
    /* empty message ? */
    if (!message || !message[0])
        return 0;
    
    /* highlight by nickname */
    match = strstr (message, nick);
    if (match)
    {
        match_pre = utf8_prev_char (message, match);
        if (!match_pre)
            match_pre = match - 1;
        match_post = match + strlen(nick);
        startswith = ((match == message) || (!irc_protocol_is_word_char (match_pre)));
        endswith = ((!match_post[0]) || (!irc_protocol_is_word_char (match_post)));
        if (startswith && endswith)
            return 1;
    }
    
    /* no highlight by nickname and "irc_highlight" is empty */
    if (!irc_cfg_irc_highlight || !irc_cfg_irc_highlight[0])
        return 0;
    
    /* convert both strings to lower case */
    if ((msg = strdup (message)) == NULL)
        return 0;
    if ((highlight = strdup (irc_cfg_irc_highlight)) == NULL)
    {
        free (msg);
        return 0;
    }
    pos = msg;
    while (pos[0])
    {
        pos[0] = tolower (pos[0]);
        pos++;
    }
    pos = highlight;
    while (pos[0])
    {
        pos[0] = tolower (pos[0]);
        pos++;
    }
    
    /* look in "irc_highlight" for highlight */
    pos = highlight;
    end = 0;
    while (!end)
    {
        pos_end = strchr (pos, ',');
        if (!pos_end)
        {
            pos_end = strchr (pos, '\0');
            end = 1;
        }
        /* error parsing string! */
        if (!pos_end)
        {
            free (msg);
            free (highlight);
            return 0;
        }
        
        length = pos_end - pos;
        pos_end[0] = '\0';
        if (length > 0)
        {
            if ((wildcard_start = (pos[0] == '*')))
            {
                pos++;
                length--;
            }
            if ((wildcard_end = (*(pos_end - 1) == '*')))
            {
                *(pos_end - 1) = '\0';
                length--;
            }
        }
            
        if (length > 0)
        {
            msg_pos = msg;
            /* highlight found! */
            while ((match = strstr (msg_pos, pos)) != NULL)
            {
                match_pre = match - 1;
                match_pre = utf8_prev_char (msg, match);
                if (!match_pre)
                    match_pre = match - 1;
                match_post = match + length;
                startswith = ((match == msg) || (!irc_protocol_is_word_char (match_pre)));
                endswith = ((!match_post[0]) || (!irc_protocol_is_word_char (match_post)));
                if ((wildcard_start && wildcard_end) ||
                    (!wildcard_start && !wildcard_end && 
                     startswith && endswith) ||
                    (wildcard_start && endswith) ||
                    (wildcard_end && startswith))
                {
                    free (msg);
                    free (highlight);
                    return 1;
                }
                msg_pos = match_post;
            }
        }
        
        if (!end)
            pos = pos_end + 1;
    }
    
    /* no highlight found with "irc_highlight" list */
    free (msg);
    free (highlight);
    return 0;
}

/*
 * irc_protocol_replace_vars: replace special IRC vars ($nick, $channel, $server) in a string
 *                            Note: result has to be free() after use
 */ 

char *
irc_protocol_replace_vars (t_irc_server *server, t_irc_channel *channel, char *string)
{
    char *var_nick, *var_channel, *var_server;
    char empty_string[1] = { '\0' };
    char *res, *temp;
    
    var_nick = (server && server->nick) ? server->nick : empty_string;
    var_channel = (channel) ? channel->name : empty_string;
    var_server = (server) ? server->name : empty_string;
    
    /* replace nick */
    temp = weechat_strreplace (string, "$nick", var_nick);
    if (!temp)
        return NULL;
    res = temp;
    
    /* replace channel */
    temp = weechat_strreplace (res, "$channel", var_channel);
    free (res);
    if (!temp)
        return NULL;
    res = temp;
    
    /* replace server */
    temp = weechat_strreplace (res, "$server", var_server);
    free (res);
    if (!temp)
        return NULL;
    res = temp;
    
    /* return result */
    return res;
}

/*
 * irc_protocol_recv_command: executes action when receiving IRC command
 *                            return:  0 = all ok, command executed
 *                                    -1 = command failed
 *                                    -2 = no command to execute
 *                                    -3 = command not found
 */

int
irc_protocol_recv_command (t_irc_server *server, char *entire_line,
                           char *host, char *command, char *arguments)
{
    int i, cmd_found, return_code, ignore, highlight;
    char *pos, *nick;
    char *dup_entire_line, *dup_host, *dup_arguments, *irc_message;
    t_irc_recv_func *cmd_recv_func;
    char *cmd_name;
    
    if (!command)
        return -2;

    /* look for IRC command */
    cmd_found = -1;
    for (i = 0; irc_protocol_messages[i].name; i++)
    {
        if (weechat_strcasecmp (irc_protocol_messages[i].name, command) == 0)
        {
            cmd_found = i;
            break;
        }
    }
    
    /* command not found */
    if (cmd_found < 0)
    {
        /* for numeric commands, we use default recv function */
        if (irc_protocol_is_numeric_command (command))
        {
            cmd_name = command;
            cmd_recv_func = irc_protocol_cmd_server_msg;
        }
        else
            return -3;
    }
    else
    {
        cmd_name = irc_protocol_messages[cmd_found].name;
        cmd_recv_func = irc_protocol_messages[cmd_found].recv_function;
    }
    
    if (cmd_recv_func != NULL)
    {
        dup_entire_line = (entire_line) ? strdup (entire_line) : NULL;
        dup_host = (host) ? strdup (host) : NULL;
        dup_arguments = (arguments) ? strdup (arguments) : NULL;
        
        ignore = 0;
        highlight = 0;
        
        return_code = plugin_msg_handler_exec (server->name,
                                               cmd_name,
                                               dup_entire_line);
        /* plugin handler choosed to discard message for WeeChat,
           so we ignore this message in standard handler */
        if (return_code & PLUGIN_RC_OK_IGNORE_WEECHAT)
            ignore = 1;
        /* plugin asked for highlight ? */
        if (return_code & PLUGIN_RC_OK_WITH_HIGHLIGHT)
            highlight = 1;
        
        pos = (dup_host) ? strchr (dup_host, '!') : NULL;
        if (pos)
            pos[0] = '\0';
        nick = (dup_host) ? strdup (dup_host) : NULL;
        if (pos)
            pos[0] = '!';
        irc_message = strdup (dup_entire_line);
        return_code = (int) (cmd_recv_func) (server, irc_message,
                                             dup_host, nick,
                                             dup_arguments,
                                             ignore, highlight);
        if (irc_message)
            free (irc_message);
        if (nick)
            free (nick);
        if (dup_entire_line)
            free (dup_entire_line);
        if (dup_host)
            free (dup_host);
        if (dup_arguments)
            free (dup_arguments);
        return return_code;
    }
    
    return 0;
}

/*
 * irc_protocol_cmd_error: error received from server
 */

int
irc_protocol_cmd_error (t_irc_server *server, char *irc_message, char *host,
                        char *nick, char *arguments, int ignore, int highlight)
{
    char *pos;
    int first;
    t_gui_buffer *ptr_buffer;
    t_irc_channel *ptr_channel;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) ignore;
    (void) highlight;
    
    first = 1;
    ptr_buffer = server->buffer;
    
    while (arguments && arguments[0])
    {
        while (arguments[0] == ' ')
            arguments++;
        
        if (arguments[0] == ':')
        {
            arguments++;
            if (first)
                gui_chat_printf_error (ptr_buffer,
                                       "%s%s%s\n",
                                       GUI_COLOR(GUI_COLOR_CHAT),
                                       (first) ? "" : ": ",
                                       arguments);
            else
                gui_chat_printf (ptr_buffer,
                                 "%s%s%s\n",
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 (first) ? "" : ": ",
                                 arguments);
            if (strncmp (arguments, "Closing Link", 12) == 0)
                irc_server_disconnect (server, 1);
            arguments = NULL;
        }
        else
        {
            pos = strchr (arguments, ' ');
            if (pos)
                pos[0] = '\0';
            if (strcasecmp (arguments, server->nick) != 0)
            {
                if (first)
                {
                    ptr_channel = irc_channel_search (server, arguments);
                    if (ptr_channel)
                        ptr_buffer = ptr_channel->buffer;
                    gui_chat_printf_error (ptr_buffer,
                                           "%s%s%s",
                                           GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                           (first) ? "" : " ",
                                           arguments);
                }
                else
                    gui_chat_printf (ptr_buffer,
                                     "%s%s%s",
                                     GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                     (first) ? "" : " ",
                                     arguments);
                first = 0;
            }
            if (pos)
                arguments = pos + 1;
            else
                arguments = NULL;
        }
    }
    
    return 0;
}

/*
 * irc_protocol_cmd_invite: 'invite' message received
 */

int
irc_protocol_cmd_invite (t_irc_server *server, char *irc_message, char *host,
                         char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_channel;

    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) highlight;
    
    pos_channel = strchr (arguments, ' ');
    if (pos_channel)
    {
        pos_channel[0] = '\0';
        pos_channel++;
        while (pos_channel[0] == ' ')
            pos_channel++;
        if (pos_channel[0] == ':')
            pos_channel++;
        
        if (!ignore)
        {
            gui_chat_printf_server (server->buffer,
                                    _("You have been invited to %s%s%s by "
                                      "%s%s\n"),
                                    GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                    pos_channel,
                                    GUI_COLOR(GUI_COLOR_CHAT),
                                    GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                    nick);
            if (gui_add_hotlist
                && ((server->buffer->num_displayed == 0)
                    || (gui_buffer_is_scrolled (server->buffer))))
            {
                gui_hotlist_add (GUI_HOTLIST_HIGHLIGHT, NULL,
                                 server->buffer, 0);
                gui_status_draw (gui_current_window->buffer, 1);
            }
        }
    }
    else
    {
        gui_chat_printf_error_nolog (server->buffer,
                                     _("%s channel \"%s\" not found for "
                                       "\"%s\" command\n"),
                                     WEECHAT_ERROR, "", "invite");
        return -1;
    }
    return 0;
}


/*
 * irc_protocol_cmd_join: 'join' message received
 */

int
irc_protocol_cmd_join (t_irc_server *server, char *irc_message, char *host,
                       char *nick, char *arguments, int ignore, int highlight)
{
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    char *pos;

    /* make C compiler happy */
    (void) irc_message;
    (void) highlight;
    
    /* no host => we can't identify sender of message! */
    if (!host)
    {
        gui_chat_printf_error_nolog (server->buffer,
                                     _("%s \"%s\" command received without "
                                       "host\n"),
                                     WEECHAT_ERROR, "join");
        return -1;
    }
    
    if (arguments[0] == ':')
        arguments++;
    
    ptr_channel = irc_channel_search (server, arguments);
    if (!ptr_channel)
    {
        ptr_channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL,
                                       arguments, 1);
        if (!ptr_channel)
        {
            gui_chat_printf_error_nolog (server->buffer,
                                         _("%s cannot create new channel "
                                           "\"%s\"\n"),
                                         WEECHAT_ERROR, arguments);
            return -1;
        }
    }
    
    pos = strchr (host, '!');
    if (!ignore)
    {
        gui_chat_printf_join (ptr_channel->buffer,
                              _("%s%s %s(%s%s%s)%s has joined %s%s\n"),
                              GUI_COLOR(GUI_COLOR_CHAT_NICK),
                              nick,
                              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                              GUI_COLOR(GUI_COLOR_CHAT_HOST),
                              (pos) ? pos + 1 : host,
                              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                              GUI_COLOR(GUI_COLOR_CHAT),
                              GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                              arguments);
    }
    
    /* remove topic and display channel creation date if joining new channel */
    if (!ptr_channel->nicks)
    {
        if (ptr_channel->topic)
        {
            free (ptr_channel->topic);
            ptr_channel->topic = NULL;
            gui_chat_draw_title (ptr_channel->buffer, 1);
        }
        ptr_channel->display_creation_date = 1;
    }
    
    /* add nick in channel */
    ptr_nick = irc_nick_new (server, ptr_channel, nick, 0, 0, 0, 0, 0, 0, 0);
    if (ptr_nick)
        ptr_nick->host = strdup ((pos) ? pos + 1 : host);

    /* redraw nicklist and status bar */
    gui_nicklist_draw (ptr_channel->buffer, 1, 1);
    gui_status_draw (ptr_channel->buffer, 1);
    return 0;
}

/*
 * irc_protocol_cmd_kick: 'kick' message received
 */

int
irc_protocol_cmd_kick (t_irc_server *server, char *irc_message, char *host,
                       char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_nick, *pos_comment;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) highlight;
    
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
        
        ptr_channel = irc_channel_search (server, arguments);
        if (!ptr_channel)
        {
            gui_chat_printf_error_nolog (server->buffer,
                                         _("%s channel \"%s\" not found for "
                                           "\"%s\" command\n"),
                                         WEECHAT_ERROR, arguments, "kick");
            return -1;
        }
        
        if (!ignore)
        {
            gui_chat_printf_quit (ptr_channel->buffer,
                                  _("%s%s%s has kicked %s%s%s from %s%s"),
                                  GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                  nick,
                                  GUI_COLOR(GUI_COLOR_CHAT),
                                  GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                  pos_nick,
                                  GUI_COLOR(GUI_COLOR_CHAT),
                                  GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                  arguments);
            if (pos_comment)
                gui_chat_printf (ptr_channel->buffer, " %s(%s%s%s)\n",
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                 GUI_COLOR(GUI_COLOR_CHAT),
                                 pos_comment,
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
            else
                gui_chat_printf (ptr_channel->buffer, "\n");
        }
    }
    else
    {
        gui_chat_printf_error_nolog (server->buffer,
                                     _("%s nick \"%s\" not found for \"%s\" "
                                       "command\n"),
                                     WEECHAT_ERROR, "", "kick");
        return -1;
    }
    if (strcmp (pos_nick, server->nick) == 0)
    {
        /* my nick was kicked => free all nicks, channel is not active any more */
        irc_nick_free_all (ptr_channel);
        gui_nicklist_draw (ptr_channel->buffer, 1, 1);
        gui_status_draw (ptr_channel->buffer, 1);
        if (server->autorejoin)
            irc_cmd_join_server (server, ptr_channel->name);
    }
    {
        /* someone was kicked from channel (but not me) => remove only this nick */
        ptr_nick = irc_nick_search (ptr_channel, pos_nick);
        if (ptr_nick)
        {
            irc_nick_free (ptr_channel, ptr_nick);
            gui_nicklist_draw (ptr_channel->buffer, 1, 1);
            gui_status_draw (ptr_channel->buffer, 1);
        }
    }
    return 0;
}

/*
 * irc_protocol_cmd_kill: 'kill' message received
 */

int
irc_protocol_cmd_kill (t_irc_server *server, char *irc_message, char *host,
                       char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_host2, *pos_comment;
    t_irc_channel *ptr_channel;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) highlight;
    
    pos_host2 = strchr (arguments, ' ');
    if (pos_host2)
    {
        pos_host2[0] = '\0';
        pos_host2++;
        while (pos_host2[0] == ' ')
            pos_host2++;
        
        if (pos_host2[0] == ':')
            pos_comment = pos_host2 + 1;
        else
        {
            pos_comment = strchr (pos_host2, ' ');
            if (pos_comment)
            {
                pos_comment[0] = '\0';
                pos_comment++;
                while (pos_comment[0] == ' ')
                    pos_comment++;
                if (pos_comment[0] == ':')
                    pos_comment++;
            }
        }
        
        for (ptr_channel = server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if (!ignore)
            {
                gui_chat_printf_quit (ptr_channel->buffer,
                                      _("%s%s%s has killed %s%s%s from "
                                        "server"),
                                      GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                      nick,
                                      GUI_COLOR(GUI_COLOR_CHAT),
                                      GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                      arguments,
                                      GUI_COLOR(GUI_COLOR_CHAT));
                if (pos_comment)
                    gui_chat_printf (ptr_channel->buffer, " %s(%s%s%s)\n",
                                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     pos_comment,
                                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
                else
                    gui_chat_printf (ptr_channel->buffer, "\n");
            }
        }
    }
    else
    {
        gui_chat_printf_error_nolog (server->buffer,
                                     _("%s host not found for \"%s\" "
                                       "command\n"),
                                     WEECHAT_ERROR, "kill");
        return -1;
    }
    return 0;
}

/*
 * irc_protocol_cmd_mode: 'mode' message received
 */

int
irc_protocol_cmd_mode (t_irc_server *server, char *irc_message, char *host,
                       char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_modes, *pos;
    t_irc_channel *ptr_channel;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) highlight;
    
    /* no host => we can't identify sender of message! */
    if (!host)
    {
        gui_chat_printf_error_nolog (server->buffer,
                                     _("%s \"%s\" command received without "
                                       "host\n"),
                                     WEECHAT_ERROR, "mode");
        return -1;
    }
    
    pos_modes = strchr (arguments, ' ');
    if (!pos_modes)
    {
        gui_chat_printf_error_nolog (server->buffer,
                                     _("%s \"%s\" command received without "
                                       "channel or nickname\n"),
                                     WEECHAT_ERROR, "mode");
        return -1;
    }
    pos_modes[0] = '\0';
    pos_modes++;
    while (pos_modes[0] == ' ')
        pos_modes++;
    if (pos_modes[0] == ':')
        pos_modes++;
    
    /* remove spaces after modes */
    pos = pos_modes + strlen (pos_modes) - 1;
    while ((pos >= pos_modes) && (pos[0] == ' '))
    {
        pos[0] = '\0';
        pos--;
    }
    
    if (irc_channel_is_channel (arguments))
    {
        ptr_channel = irc_channel_search (server, arguments);
        if (ptr_channel)
        {
            if (!ignore)
            {
                gui_chat_printf_info (ptr_channel->buffer,
                                      _("Mode %s%s %s[%s%s%s]%s by %s%s\n"),
                                      GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                      ptr_channel->name,
                                      GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                      GUI_COLOR(GUI_COLOR_CHAT),
                                      pos_modes,
                                      GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                      GUI_COLOR(GUI_COLOR_CHAT),
                                      GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                      nick);
            }
            irc_mode_channel_set (server, ptr_channel, pos_modes);
            irc_server_sendf (server, "MODE %s", ptr_channel->name);
        }
        else
        {
            gui_chat_printf_error_nolog (server->buffer,
                                         _("%s channel \"%s\" not found for "
                                           "\"%s\" command\n"),
                                         WEECHAT_ERROR, arguments, "mode");
            return -1;
        }
    }
    else
    {
        if (!ignore)
        {
            gui_chat_printf_info (server->buffer,
                                  _("User mode %s[%s%s%s]%s by %s%s\n"),
                                  GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                  GUI_COLOR(GUI_COLOR_CHAT),
                                  pos_modes,
                                  GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                  GUI_COLOR(GUI_COLOR_CHAT),
                                  GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                  nick);
        }
        irc_mode_user_set (server, pos_modes);
    }
    return 0;
}

/*
 * irc_protocol_cmd_nick: 'nick' message received
 */

int
irc_protocol_cmd_nick (t_irc_server *server, char *irc_message, char *host,
                       char *nick, char *arguments, int ignore, int highlight)
{
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    int nick_is_me;
    t_gui_window *ptr_window;

    /* make C compiler happy */
    (void) irc_message;
    (void) highlight;
    
    /* no host => we can't identify sender of message! */
    if (!host)
    {
        gui_chat_printf_error_nolog (server->buffer,
                                     _("%s \"%s\" command received without "
                                       "host\n"),
                                     WEECHAT_ERROR, "nick");
        return -1;
    }

    if (arguments[0] == ':')
        arguments++;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        switch (ptr_channel->type)
        {
            case IRC_CHANNEL_TYPE_PRIVATE:
            case IRC_CHANNEL_TYPE_DCC_CHAT:
                /* rename private window if this is with "old nick" */
                if (weechat_strcasecmp (ptr_channel->name, nick) == 0)
                {
                    free (ptr_channel->name);
                    ptr_channel->name = strdup (arguments);
                }
                break;
            case IRC_CHANNEL_TYPE_CHANNEL:
                /* rename nick in nicklist if found */
                ptr_nick = irc_nick_search (ptr_channel, nick);
                if (ptr_nick)
                {
                    nick_is_me = (strcmp (ptr_nick->nick, server->nick) == 0) ? 1 : 0;
                    if (nick_is_me)
                        gui_add_hotlist = 0;
                    irc_nick_change (server, ptr_channel, ptr_nick, arguments);
                    if (!ignore)
                    {
                        if (nick_is_me)
                            gui_chat_printf_info (ptr_channel->buffer,
                                                  _("You are now known as "
                                                    "%s%s\n"),
                                                  GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                                  arguments);
                        else
                            gui_chat_printf_info (ptr_channel->buffer,
                                                  _("%s%s%s is now known as "
                                                    "%s%s\n"),
                                                  GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                                  nick,
                                                  GUI_COLOR(GUI_COLOR_CHAT),
                                                  GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                                  arguments);
                    }
                    gui_nicklist_draw (ptr_channel->buffer, 1, 1);
                    gui_add_hotlist = 1;
                }
                break;
        }
    }
    
    if (strcmp (server->nick, nick) == 0)
    {
        free (server->nick);
        server->nick = strdup (arguments);
        gui_status_draw (gui_current_window->buffer, 1);
        for (ptr_window = gui_windows; ptr_window;
             ptr_window = ptr_window->next_window)
        {
            if ((ptr_window->buffer->protocol == irc_protocol)
                && (IRC_BUFFER_SERVER(ptr_window->buffer) == server))
                gui_input_draw (ptr_window->buffer, 1);
        }
    }
    else
    {
        gui_status_draw (gui_current_window->buffer, 1);
        gui_input_draw (gui_current_window->buffer, 1);
    }
    
    return 0;
}

/*
 * irc_protocol_cmd_notice: 'notice' message received
 */

int
irc_protocol_cmd_notice (t_irc_server *server, char *irc_message, char *host,
                         char *nick, char *arguments, int ignore, int highlight)
{
    char *host2, *pos, *pos2, *pos_usec;
    struct timeval tv;
    long sec1, usec1, sec2, usec2, difftime;
    t_irc_channel *ptr_channel;
    int highlight_displayed;
    
    /* make C compiler happy */
    (void) irc_message;
    
    host2 = NULL;
    if (host)
    {
        pos = strchr (host, '!');
        if (pos)
            host2 = pos + 1;
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
        gui_chat_printf_error_nolog (server->buffer,
                                     _("%s nickname not found for \"%s\" "
                                       "command\n"),
                                     WEECHAT_ERROR, "notice");
        return -1;
    }
    
    if (!ignore)
    {
        if (strncmp (pos, "\01VERSION", 8) == 0)
        {
            pos += 9;
            pos2 = strchr (pos, '\01');
            if (pos2)
                pos2[0] = '\0';
            gui_chat_printf_server (server->buffer,
                                    _("CTCP %sVERSION%s reply from %s%s%s: "
                                      "%s\n"),
                                    GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                    GUI_COLOR(GUI_COLOR_CHAT),
                                    GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                    nick,
                                    GUI_COLOR(GUI_COLOR_CHAT),
                                    pos);
        }
        else
        {
            if (strncmp (pos, "\01PING", 5) == 0)
            {
                pos += 5;
                while (pos[0] == ' ')
                    pos++;
                pos_usec = strchr (pos, ' ');
                if (pos_usec)
                {
                    pos_usec[0] = '\0';
                    pos_usec++;
                    pos2 = strchr (pos_usec, '\01');
                    if (pos2)
                    {
                        pos2[0] = '\0';
                        
                        gettimeofday (&tv, NULL);
                        sec1 = atol (pos);
                        usec1 = atol (pos_usec);
                        sec2 = tv.tv_sec;
                        usec2 = tv.tv_usec;
                        
                        difftime = ((sec2 * 1000000) + usec2) -
                            ((sec1 * 1000000) + usec1);
                        
                        gui_chat_printf_server (server->buffer,
                                                _("CTCP %sPING%s reply from "
                                                  "%s%s%s: %ld.%ld seconds\n"),
                                                GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                                GUI_COLOR(GUI_COLOR_CHAT),
                                                GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                                nick,
                                                GUI_COLOR(GUI_COLOR_CHAT),
                                                difftime / 1000000,
                                                (difftime % 1000000) / 1000);
                    }
                }
            }
            else
            {
                if (nick && nick[0] && irc_cfg_irc_notice_as_pv)
                {
                    ptr_channel = irc_channel_search (server, nick);
                    if (!ptr_channel)
                    {
                        ptr_channel = irc_channel_new (server,
                                                       IRC_CHANNEL_TYPE_PRIVATE,
                                                       nick, 0);
                        if (!ptr_channel)
                        {
                            gui_chat_printf_error_nolog (server->buffer,
                                                         _("%s cannot create "
                                                           "new private "
                                                           "window \"%s\"\n"),
                                                         WEECHAT_ERROR, nick);
                            return -1;
                        }
                    }
                    if (!ptr_channel->topic)
                    {
                        ptr_channel->topic = strdup ((host2) ? host2 : "");
                        gui_chat_draw_title (ptr_channel->buffer, 1);
                    }
                    
                    gui_chat_printf_type (ptr_channel->buffer, GUI_MSG_TYPE_NICK,
                                          NULL, -1, "%s<",
                                          GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
                    if (highlight
                        || irc_protocol_is_highlight (pos, server->nick))
                    {
                        gui_chat_printf_type (ptr_channel->buffer,
                                              GUI_MSG_TYPE_NICK |
                                              GUI_MSG_TYPE_HIGHLIGHT,
                                              NULL, -1, "%s%s",
                                              GUI_COLOR(GUI_COLOR_CHAT_HIGHLIGHT),
                                              nick);
                        if ( (cfg_look_infobar_delay_highlight > 0)
                             && (ptr_channel->buffer != gui_current_window->buffer) )
                            gui_infobar_printf (cfg_look_infobar_delay_highlight,
                                                GUI_COLOR_INFOBAR_HIGHLIGHT,
                                                _("Private %s> %s"),
                                                nick, pos);
                        highlight_displayed = 1;
                    }
                    else
                    {
                        gui_chat_printf_type (ptr_channel->buffer, GUI_MSG_TYPE_NICK,
                                              NULL, -1, "%s%s",
                                              GUI_COLOR(GUI_COLOR_CHAT_NICK_OTHER),
                                              nick);
                        highlight_displayed = 0;
                    }
                    gui_chat_printf_type (ptr_channel->buffer, GUI_MSG_TYPE_NICK,
                                          NULL, -1, "%s> ",
                                          GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
                    gui_chat_printf_type (ptr_channel->buffer, GUI_MSG_TYPE_MSG,
                                          NULL, -1, "%s%s\n",
                                          GUI_COLOR(GUI_COLOR_CHAT),
                                          pos);
                    if (highlight_displayed)
                        (void) plugin_msg_handler_exec (server->name,
                                                        "weechat_highlight",
                                                        irc_message);
                }
                else
                {
                    if (host)
                    {
                        gui_chat_printf_server (server->buffer, "%s%s",
                                                GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                                nick);
                        if (host2)
                            gui_chat_printf (server->buffer, " %s(%s%s%s)",
                                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                             GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                             host2,
                                             GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
                        gui_chat_printf (server->buffer, "%s: ",
                                         GUI_COLOR(GUI_COLOR_CHAT));
                        gui_chat_printf (server->buffer, "%s%s\n",
                                         GUI_COLOR(GUI_COLOR_CHAT),
                                         pos);
                    }
                    else
                        gui_chat_printf_server (server->buffer, "%s%s\n",
                                                GUI_COLOR(GUI_COLOR_CHAT),
                                                pos);

                    if ((nick)
                        && (weechat_strcasecmp (nick, "nickserv") != 0)
                        && (weechat_strcasecmp (nick, "chanserv") != 0)
                        && (weechat_strcasecmp (nick, "memoserv") != 0))
                    {
                        if (gui_add_hotlist
                            && ((server->buffer->num_displayed == 0)
                                || (gui_buffer_is_scrolled (server->buffer))))
                        {
                            gui_hotlist_add (GUI_HOTLIST_PRIVATE, NULL,
                                             server->buffer, 0);
                            gui_status_draw (gui_current_window->buffer, 1);
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/*
 * irc_protocol_cmd_part: 'part' message received
 */

int
irc_protocol_cmd_part (t_irc_server *server, char *irc_message, char *host,
                       char *nick, char *arguments, int ignore, int highlight)
{
    char *pos, *pos_args, *join_string;
    int join_length;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) highlight;
    
    /* no host => we can't identify sender of message! */
    if (!host || !arguments)
    {
        gui_chat_printf_error_nolog (server->buffer,
                                     _("%s \"%s\" command received without "
                                       "host or channel\n"),
                                     WEECHAT_ERROR, "part");
        return -1;
    }
    
    if (arguments[0] == ':')
        arguments++;
    
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
    
    ptr_channel = irc_channel_search (server, arguments);
    if (ptr_channel)
    {
        ptr_nick = irc_nick_search (ptr_channel, nick);
        if (ptr_nick)
        {
            /* display part message */
            if (!ignore)
            {
                pos = strchr (host, '!');
                gui_chat_printf_quit (ptr_channel->buffer,
                                      _("%s%s %s(%s%s%s)%s has left %s%s"),
                                      GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                      nick,
                                      GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                      GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                      (pos) ? pos + 1 : "",
                                      GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                      GUI_COLOR(GUI_COLOR_CHAT),
                                      GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                      ptr_channel->name);
                if (pos_args && pos_args[0])
                    gui_chat_printf (ptr_channel->buffer, " %s(%s%s%s)\n",
                                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     pos_args,
                                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
                else
                    gui_chat_printf (ptr_channel->buffer, "\n");
            }
            
            /* part request was issued by local client ? */
            if (strcmp (ptr_nick->nick, server->nick) == 0)
            {
                irc_nick_free_all (ptr_channel);
                
                /* cycling ? => rejoin channel immediately */
                if (ptr_channel->cycle)
                {
                    ptr_channel->cycle = 0;
                    if (ptr_channel->key)
                    {
                        join_length = strlen (ptr_channel->name) + 1 +
                            strlen (ptr_channel->key) + 1;
                        join_string = (char *)malloc (join_length);
                        if (join_string)
                        {
                            snprintf (join_string, join_length, "%s %s",
                                      ptr_channel->name,
                                      ptr_channel->key);
                            irc_cmd_join_server (server, join_string);
                            free (join_string);
                        }
                        else
                            irc_cmd_join_server (server, ptr_channel->name);
                    }
                    else
                        irc_cmd_join_server (server, ptr_channel->name);
                }
                if (ptr_channel->close)
                {
                    gui_buffer_free (ptr_channel->buffer, 1);
                    irc_channel_free (server, ptr_channel);
                    ptr_channel = NULL;
                }
            }
            else
                irc_nick_free (ptr_channel, ptr_nick);
            
            if (ptr_channel)
            {
                gui_nicklist_draw (ptr_channel->buffer, 1, 1);
                gui_status_draw (ptr_channel->buffer, 1);
            }
            gui_input_draw (gui_current_window->buffer, 1);
        }
    }
    else
    {
        gui_chat_printf_error_nolog (server->buffer,
                                     _("%s channel \"%s\" not found for "
                                       "\"%s\" command\n"),
                                     WEECHAT_ERROR, arguments, "part");
        return -1;
    }
    
    return 0;
}

/*
 * irc_protocol_cmd_ping: 'ping' command received
 */

int
irc_protocol_cmd_ping (t_irc_server *server, char *irc_message, char *host,
                       char *nick, char *arguments, int ignore, int highlight)
{
    char *pos;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) ignore;
    (void) highlight;

    if (arguments[0] == ':')
        arguments++;
    
    pos = strrchr (arguments, ' ');
    if (pos)
        pos[0] = '\0';
    
    irc_server_sendf (server, "PONG :%s", arguments);
    
    return 0;
}

/*
 * irc_protocol_cmd_pong: 'pong' command received
 */

int
irc_protocol_cmd_pong (t_irc_server *server, char *irc_message, char *host,
                       char *nick, char *arguments, int ignore, int highlight)
{
    struct timeval tv;
    int old_lag;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) arguments;
    (void) ignore;
    (void) highlight;
    
    if (server->lag_check_time.tv_sec != 0)
    {
        /* calculate lag (time diff with lag check) */
        old_lag = server->lag;
        gettimeofday (&tv, NULL);
        server->lag = (int) weechat_get_timeval_diff (&(server->lag_check_time), &tv);
        if (old_lag != server->lag)
            gui_status_draw (gui_current_window->buffer, 1);
        
        /* schedule next lag check */
        server->lag_check_time.tv_sec = 0;
        server->lag_check_time.tv_usec = 0;
        server->lag_next_check = time (NULL) + irc_cfg_irc_lag_check;
    }
    return 0;
}

/*
 * irc_cmd_reply_version: send version in reply to "CTCP VERSION" request
 */

void
irc_cmd_reply_version (t_irc_server *server, t_irc_channel *channel,
                       char *nick, char *message, int ignore)
{
    char *pos;
    struct utsname *buf;
    t_gui_buffer *ptr_buffer;
    
    ptr_buffer = (channel) ? channel->buffer : server->buffer;
    
    if (!ignore)
    {
        pos = strchr (message, ' ');
        if (pos)
        {
            while (pos[0] == ' ')
                pos++;
            if (pos[0] == '\01')
                pos = NULL;
            else if (!pos[0])
                pos = NULL;
        }
        
        buf = (struct utsname *) malloc (sizeof (struct utsname));
        if (buf && (uname (buf) >= 0))
        {
            irc_server_sendf (server,
                              "NOTICE %s :%sVERSION %s v%s"
                              " compiled on %s, running "
                              "%s %s / %s%s",
                              nick, "\01", PACKAGE_NAME, PACKAGE_VERSION, __DATE__,
                              &buf->sysname,
                              &buf->release, &buf->machine, "\01");
            free (buf);
        }
        else
            irc_server_sendf (server,
                              "NOTICE %s :%sVERSION %s v%s"
                              " compiled on %s%s",
                              nick, "\01", PACKAGE_NAME, PACKAGE_VERSION, __DATE__,
                              "\01");
        gui_chat_printf_server (ptr_buffer,
                                _("CTCP %sVERSION%s received from %s%s"),
                                GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                GUI_COLOR(GUI_COLOR_CHAT),
                                GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                nick);
        if (pos)
            gui_chat_printf (ptr_buffer, "%s: %s\n",
                             GUI_COLOR(GUI_COLOR_CHAT),
                             pos);
        else
            gui_chat_printf (ptr_buffer, "\n");
        (void) plugin_msg_handler_exec (server->name,
                                        "weechat_ctcp",
                                        irc_message);
    }
}

/*
 * irc_protocol_cmd_privmsg: 'privmsg' command received
 */

int
irc_protocol_cmd_privmsg (t_irc_server *server, char *irc_message, char *host,
                          char *nick, char *arguments, int ignore, int highlight)
{
    char *pos, *pos2, *host2;
    char *pos_file, *pos_addr, *pos_port, *pos_size, *pos_start_resume;  /* for DCC */
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    int highlight_displayed;
    
    /* make C compiler happy */
    (void) irc_message;
    
    /* no host => we can't identify sender of message! */
    if (!host)
    {
        gui_chat_printf_error_nolog (server->buffer,
                                     _("%s \"%s\" command received without "
                                       "host\n"),
                                     WEECHAT_ERROR, "privmsg");
        return -1;
    }
    
    pos = strchr (host, '!');
    if (pos)
        host2 = pos + 1;
    else
        host2 = host;
    
    /* receiver is a channel ? */
    if (irc_channel_is_channel (arguments))
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
            
            ptr_channel = irc_channel_search (server, arguments);
            if (ptr_channel)
            {
                if (strncmp (pos, "\01ACTION ", 8) == 0)
                {
                    pos += 8;
                    pos2 = strchr (pos, '\01');
                    if (pos2)
                        pos2[0] = '\0';
                    if (!ignore)
                    {
                        if (highlight
                            || irc_protocol_is_highlight (pos, server->nick))
                        {
                            gui_chat_printf_type (ptr_channel->buffer,
                                                  GUI_MSG_TYPE_MSG |
                                                  GUI_MSG_TYPE_HIGHLIGHT,
                                                  cfg_look_prefix_action,
                                                  cfg_col_chat_prefix_action,
                                                  "%s%s",
                                                  GUI_COLOR(GUI_COLOR_CHAT_HIGHLIGHT),
                                                  nick);
                            if ( (cfg_look_infobar)
                                 && (cfg_look_infobar_delay_highlight > 0)
                                 && (ptr_channel->buffer != gui_current_window->buffer) )
                                gui_infobar_printf (cfg_look_infobar_delay_highlight,
                                                    GUI_COLOR_INFOBAR_HIGHLIGHT,
                                                    _("Channel %s: * %s %s"),
                                                    ptr_channel->name, nick, pos);
                            gui_chat_printf (ptr_channel->buffer, " %s%s\n",
                                        GUI_COLOR(GUI_COLOR_CHAT), pos);
                            (void) plugin_msg_handler_exec (server->name,
                                                            "weechat_highlight",
                                                            irc_message);
                        }
                        else
                        {
                            gui_chat_printf_type (ptr_channel->buffer,
                                                  GUI_MSG_TYPE_MSG,
                                                  NULL, -1,
                                                  "%s%s",
                                                  GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                                  cfg_look_prefix_action,
                                                  cfg_col_chat_prefix_action,
                                                  nick);
                            gui_chat_printf (ptr_channel->buffer, " %s%s\n",
                                             GUI_COLOR(GUI_COLOR_CHAT), pos);
                        }
                        irc_channel_add_nick_speaking (ptr_channel, nick);
                    }
                    return 0;
                }
                if (strncmp (pos, "\01SOUND ", 7) == 0)
                {
                    pos += 7;
                    pos2 = strchr (pos, '\01');
                    if (pos2)
                        pos2[0] = '\0';
                    if (!ignore)
                    {
                        gui_chat_printf_server (ptr_channel->buffer,
                                                _("Received a CTCP %sSOUND%s "
                                                  "\"%s\" from %s%s\n"),
                                                GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                                GUI_COLOR(GUI_COLOR_CHAT),
                                                pos,
                                                GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                                nick);
                    }
                    return 0;
                }
                if (strncmp (pos, "\01PING", 5) == 0)
                {
                    pos += 5;
                    while (pos[0] == ' ')
                        pos++;
                    pos2 = strchr (pos, '\01');
                    if (pos2)
                        pos2[0] = '\0';
                    else
                        pos = NULL;
                    if (pos && !pos[0])
                        pos = NULL;
                    if (pos)
                        irc_server_sendf (server, "NOTICE %s :\01PING %s\01",
                                          nick, pos);
                    else
                        irc_server_sendf (server, "NOTICE %s :\01PING\01",
                                          nick);
                    gui_chat_printf_server (ptr_channel->buffer,
                                            _("CTCP %sPING%s received from "
                                              "%s%s\n"),
                                            GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                            GUI_COLOR(GUI_COLOR_CHAT),
                                            GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                            nick);
                    return 0;
                }
                if (strncmp (pos, "\01VERSION", 8) == 0)
                {
                    irc_cmd_reply_version (server, ptr_channel, nick,
                                           pos, ignore);
                    return 0;
                }
                
                /* unknown CTCP ? */
                pos2 = strchr (pos + 1, '\01');
                if ((pos[0] == '\01') && pos2 && (pos2[1] == '\0'))
                {
                    pos++;
                    pos2[0] = '\0';
                    pos2 = strchr (pos, ' ');
                    if (pos2)
                    {
                        pos2[0] = '\0';
                        pos2++;
                        while (pos2[0] == ' ')
                            pos2++;
                        if (!pos2[0])
                            pos2 = NULL;
                    }
                    if (!ignore)
                    {
                        gui_chat_printf_server (ptr_channel->buffer,
                                                _("Unknown CTCP %s%s%s "
                                                  "received from %s%s"),
                                                GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                                pos,
                                                GUI_COLOR(GUI_COLOR_CHAT),
                                                GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                                nick);
                        if (pos2)
                            gui_chat_printf (ptr_channel->buffer, "%s: %s\n",
                                             GUI_COLOR(GUI_COLOR_CHAT),
                                             pos2);
                        else
                            gui_chat_printf (ptr_channel->buffer, "\n");
                    }
                    return 0;
                }
                
                /* other message */
                if (!ignore)
                {
                    ptr_nick = irc_nick_search (ptr_channel, nick);
                    if (highlight || irc_protocol_is_highlight (pos, server->nick))
                    {
                        irc_display_nick (ptr_channel->buffer, ptr_nick,
                                          (ptr_nick) ? NULL : nick,
                                          GUI_MSG_TYPE_NICK | GUI_MSG_TYPE_HIGHLIGHT,
                                          1, GUI_COLOR(GUI_COLOR_CHAT_HIGHLIGHT), 0);
                        if ( (cfg_look_infobar)
                             && (cfg_look_infobar_delay_highlight > 0)
                             && (ptr_channel->buffer != gui_current_window->buffer) )
                            gui_infobar_printf (cfg_look_infobar_delay_highlight,
                                                GUI_COLOR_INFOBAR_HIGHLIGHT,
                                                _("Channel %s: %s> %s"),
                                                ptr_channel->name, nick, pos);
                        gui_chat_printf_type (ptr_channel->buffer,
                                              GUI_MSG_TYPE_MSG,
                                              NULL, -1,
                                              "%s\n", pos);
                        (void) plugin_msg_handler_exec (server->name,
                                                        "weechat_highlight",
                                                        irc_message);
                    }
                    else
                    {
                        irc_display_nick (ptr_channel->buffer, ptr_nick,
                                          (ptr_nick) ? NULL : nick,
                                          GUI_MSG_TYPE_NICK, 1, NULL, 0);
                        gui_chat_printf_type (ptr_channel->buffer,
                                              GUI_MSG_TYPE_MSG,
                                              NULL, -1,
                                              "%s\n", pos);
                    }
                    irc_channel_add_nick_speaking (ptr_channel, nick);
                }
            }
            else
            {
                gui_chat_printf_error_nolog (server->buffer,
                                             _("%s channel \"%s\" not found "
                                               "for \"%s\" command\n"),
                                             WEECHAT_ERROR, arguments,
                                             "privmsg");
                return -1;
            }
        }
    }
    else
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
            
            /* version asked by another user => answer with WeeChat version */
            if (strncmp (pos, "\01VERSION", 8) == 0)
            {
                irc_cmd_reply_version (server, NULL, nick, pos, ignore);
                return 0;
            }
            
            /* ping request from another user => answer */
            if (strncmp (pos, "\01PING", 5) == 0)
            {
                if (!ignore)
                {
                    pos += 5;
                    while (pos[0] == ' ')
                        pos++;
                    pos2 = strchr (pos, '\01');
                    if (pos2)
                        pos2[0] = '\0';
                    else
                        pos = NULL;
                    if (pos && !pos[0])
                        pos = NULL;
                    if (pos)
                        irc_server_sendf (server, "NOTICE %s :\01PING %s\01",
                                          nick, pos);
                    else
                        irc_server_sendf (server, "NOTICE %s :\01PING\01",
                                          nick);
                    gui_chat_printf_server (server->buffer,
                                            _("CTCP %sPING%s received from "
                                              "%s%s\n"),
                                            GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                            GUI_COLOR(GUI_COLOR_CHAT),
                                            GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                            nick);
                    (void) plugin_msg_handler_exec (server->name,
                                                    "weechat_ctcp",
                                                    irc_message);
                }
                return 0;
            }
            
            /* incoming DCC file */
            if (strncmp (pos, "\01DCC SEND", 9) == 0)
            {
                /* check if DCC SEND is ok, i.e. with 0x01 at end */
                pos2 = strchr (pos + 1, '\01');
                if (!pos2)
                {
                    gui_chat_printf_error_nolog (server->buffer,
                                                 _("%s cannot parse \"%s\" "
                                                   "command\n"),
                                                 WEECHAT_ERROR, "privmsg");
                    return -1;
                }
                pos2[0] = '\0';
                
                if (!ignore)
                {
                    /* DCC filename */
                    pos_file = pos + 9;
                    while (pos_file[0] == ' ')
                        pos_file++;
                    
                    /* look for file size */
                    pos_size = strrchr (pos_file, ' ');
                    if (!pos_size)
                    {
                        gui_chat_printf_error_nolog (server->buffer,
                                                     _("%s cannot parse "
                                                       "\"%s\" command\n"),
                                                     WEECHAT_ERROR, "privmsg");
                        return -1;
                    }
                    pos2 = pos_size;
                    pos_size++;
                    while (pos2[0] == ' ')
                        pos2--;
                    pos2[1] = '\0';
                    
                    /* look for DCC port */
                    pos_port = strrchr (pos_file, ' ');
                    if (!pos_port)
                    {
                        gui_chat_printf_error_nolog (server->buffer,
                                                     _("%s cannot parse "
                                                       "\"%s\" command\n"),
                                                     WEECHAT_ERROR, "privmsg");
                        return -1;
                    }
                    pos2 = pos_port;
                    pos_port++;
                    while (pos2[0] == ' ')
                        pos2--;
                    pos2[1] = '\0';
                    
                    /* look for DCC IP address */
                    pos_addr = strrchr (pos_file, ' ');
                    if (!pos_addr)
                    {
                        gui_chat_printf_error_nolog (server->buffer,
                                                     _("%s cannot parse "
                                                       "\"%s\" command\n"),
                                                     WEECHAT_ERROR, "privmsg");
                        return -1;
                    }
                    pos2 = pos_addr;
                    pos_addr++;
                    while (pos2[0] == ' ')
                        pos2--;
                    pos2[1] = '\0';
                    
                    irc_dcc_add (server, IRC_DCC_FILE_RECV,
                                 strtoul (pos_addr, NULL, 10),
                                 atoi (pos_port), nick, -1, pos_file, NULL,
                                 strtoul (pos_size, NULL, 10));
                    (void) plugin_msg_handler_exec (server->name,
                                                    "weechat_dcc",
                                                    irc_message);
                }
                return 0;
            }
            
            /* incoming DCC RESUME (asked by receiver) */
            if (strncmp (pos, "\01DCC RESUME", 11) == 0)
            {
                /* check if DCC RESUME is ok, i.e. with 0x01 at end */
                pos2 = strchr (pos + 1, '\01');
                if (!pos2)
                {
                    gui_chat_printf_error_nolog (server->buffer,
                                                 _("%s cannot parse \"%s\" "
                                                   "command\n"),
                                                 WEECHAT_ERROR, "privmsg");
                    return -1;
                }
                pos2[0] = '\0';
                
                if (!ignore)
                {
                    /* DCC filename */
                    pos_file = pos + 11;
                    while (pos_file[0] == ' ')
                        pos_file++;
                    
                    /* look for resume start position */
                    pos_start_resume = strrchr (pos_file, ' ');
                    if (!pos_start_resume)
                    {
                        gui_chat_printf_error_nolog (server->buffer,
                                                     _("%s cannot parse "
                                                       "\"%s\" command\n"),
                                                     WEECHAT_ERROR, "privmsg");
                        return -1;
                    }
                    pos2 = pos_start_resume;
                    pos_start_resume++;
                    while (pos2[0] == ' ')
                        pos2--;
                    pos2[1] = '\0';
                    
                    /* look for DCC port */
                    pos_port = strrchr (pos_file, ' ');
                    if (!pos_port)
                    {
                        gui_chat_printf_error_nolog (server->buffer,
                                                     _("%s cannot parse "
                                                       "\"%s\" command\n"),
                                                     WEECHAT_ERROR, "privmsg");
                        return -1;
                    }
                    pos2 = pos_port;
                    pos_port++;
                    while (pos2[0] == ' ')
                        pos2--;
                    pos2[1] = '\0';
                    
                    irc_dcc_accept_resume (server, pos_file, atoi (pos_port),
                                           strtoul (pos_start_resume, NULL, 10));
                    (void) plugin_msg_handler_exec (server->name,
                                                    "weechat_dcc",
                                                    irc_message);
                }
                return 0;
            }
            
            /* incoming DCC ACCEPT (resume accepted by sender) */
            if (strncmp (pos, "\01DCC ACCEPT", 11) == 0)
            {
                /* check if DCC ACCEPT is ok, i.e. with 0x01 at end */
                pos2 = strchr (pos + 1, '\01');
                if (!pos2)
                {
                    gui_chat_printf_error_nolog (server->buffer,
                                                 _("%s cannot parse "
                                                   "\"%s\" command\n"),
                                                 WEECHAT_ERROR, "privmsg");
                    return -1;
                }
                pos2[0] = '\0';
                
                if (!ignore)
                {
                    /* DCC filename */
                    pos_file = pos + 11;
                    while (pos_file[0] == ' ')
                        pos_file++;
                    
                    /* look for resume start position */
                    pos_start_resume = strrchr (pos_file, ' ');
                    if (!pos_start_resume)
                    {
                        gui_chat_printf_error_nolog (server->buffer,
                                                     _("%s cannot parse "
                                                       "\"%s\" command\n"),
                                                     WEECHAT_ERROR, "privmsg");
                        return -1;
                    }
                    pos2 = pos_start_resume;
                    pos_start_resume++;
                    while (pos2[0] == ' ')
                        pos2--;
                    pos2[1] = '\0';
                    
                    /* look for DCC port */
                    pos_port = strrchr (pos_file, ' ');
                    if (!pos_port)
                    {
                        gui_chat_printf_error_nolog (server->buffer,
                                                     _("%s cannot parse "
                                                       "\"%s\" command\n"),
                                                     WEECHAT_ERROR, "privmsg");
                        return -1;
                    }
                    pos2 = pos_port;
                    pos_port++;
                    while (pos2[0] == ' ')
                        pos2--;
                    pos2[1] = '\0';
                    
                    irc_dcc_start_resume (server, pos_file, atoi (pos_port),
                                          strtoul (pos_start_resume, NULL, 10));
                    (void) plugin_msg_handler_exec (server->name,
                                                    "weechat_dcc",
                                                    irc_message);
                }
                return 0;
            }
            
            /* incoming DCC CHAT */
            if (strncmp (pos, "\01DCC CHAT", 9) == 0)
            {
                /* check if DCC CHAT is ok, i.e. with 0x01 at end */
                pos2 = strchr (pos + 1, '\01');
                if (!pos2)
                {
                    gui_chat_printf_error_nolog (server->buffer,
                                                 _("%s cannot parse "
                                                   "\"%s\" command\n"),
                                                 WEECHAT_ERROR, "privmsg");
                    return -1;
                }
                pos2[0] = '\0';
                
                if (!ignore)
                {
                    /* CHAT type */
                    pos_file = pos + 9;
                    while (pos_file[0] == ' ')
                        pos_file++;
                    
                    /* DCC IP address */
                    pos_addr = strchr (pos_file, ' ');
                    if (!pos_addr)
                    {
                        gui_chat_printf_error_nolog (server->buffer,
                                                     _("%s cannot parse "
                                                       "\"%s\" command\n"),
                                                     WEECHAT_ERROR, "privmsg");
                        return -1;
                    }
                    pos_addr[0] = '\0';
                    pos_addr++;
                    while (pos_addr[0] == ' ')
                        pos_addr++;
                    
                    /* look for DCC port */
                    pos_port = strchr (pos_addr, ' ');
                    if (!pos_port)
                    {
                        gui_chat_printf_error_nolog (server->buffer,
                                                     _("%s cannot parse "
                                                       "\"%s\" command\n"),
                                                     WEECHAT_ERROR, "privmsg");
                        return -1;
                    }
                    pos_port[0] = '\0';
                    pos_port++;
                    while (pos_port[0] == ' ')
                        pos_port++;
                    
                    if (weechat_strcasecmp (pos_file, "chat") != 0)
                    {
                        gui_chat_printf_error_nolog (server->buffer,
                                                     _("%s unknown DCC CHAT "
                                                       "type received from "),
                                                     WEECHAT_ERROR);
                        gui_chat_printf (server->buffer, "%s%s%s: \"%s\"\n",
                                         GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                         nick,
                                         GUI_COLOR(GUI_COLOR_CHAT),
                                         pos_file);
                        return -1;
                    }
                    
                    irc_dcc_add (server, IRC_DCC_CHAT_RECV,
                                 strtoul (pos_addr, NULL, 10),
                                 atoi (pos_port), nick, -1, NULL, NULL, 0);
                    (void) plugin_msg_handler_exec (server->name,
                                                    "weechat_dcc",
                                                    irc_message);
                }
                return 0;
            }
            
            /* private message received => display it */
            ptr_channel = irc_channel_search (server, nick);
            
            if (strncmp (pos, "\01ACTION ", 8) == 0)
            {
                if (!ignore)
                {
                    if (!ptr_channel)
                    {
                        ptr_channel = irc_channel_new (server,
                                                       IRC_CHANNEL_TYPE_PRIVATE,
                                                       nick, 0);
                        if (!ptr_channel)
                        {
                            gui_chat_printf_error_nolog (server->buffer,
                                                         _("%s cannot create "
                                                           "new private "
                                                           "window \"%s\"\n"),
                                                         WEECHAT_ERROR, nick);
                            return -1;
                        }
                    }
                    if (!ptr_channel->topic)
                    {
                        ptr_channel->topic = strdup (host2);
                        gui_chat_draw_title (ptr_channel->buffer, 1);
                    }
                    
                    pos += 8;
                    pos2 = strchr (pos, '\01');
                    if (pos2)
                        pos2[0] = '\0';
                    if (highlight || irc_protocol_is_highlight (pos, server->nick))
                    {
                        gui_chat_printf_type (ptr_channel->buffer,
                                              GUI_MSG_TYPE_MSG |
                                              GUI_MSG_TYPE_HIGHLIGHT,
                                              cfg_look_prefix_action,
                                              cfg_col_chat_prefix_action,
                                              "%s%s",
                                              GUI_COLOR(GUI_COLOR_CHAT_HIGHLIGHT),
                                              nick);
                        if ( (cfg_look_infobar)
                             && (cfg_look_infobar_delay_highlight > 0)
                             && (ptr_channel->buffer != gui_current_window->buffer) )
                            gui_infobar_printf (cfg_look_infobar_delay_highlight,
                                                GUI_COLOR_INFOBAR_HIGHLIGHT,
                                                _("Channel %s: * %s %s"),
                                                ptr_channel->name, nick, pos);
                        gui_chat_printf (ptr_channel->buffer, " %s%s\n",
                                         GUI_COLOR(GUI_COLOR_CHAT), pos);
                        (void) plugin_msg_handler_exec (server->name,
                                                        "weechat_highlight",
                                                        irc_message);
                    }
                    else
                    {
                        gui_chat_printf_type (ptr_channel->buffer,
                                              GUI_MSG_TYPE_MSG,
                                              NULL, -1,
                                              "%s%s",
                                              GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                              cfg_look_prefix_action,
                                              cfg_col_chat_prefix_action,
                                              nick);
                        gui_chat_printf (ptr_channel->buffer, " %s%s\n",
                                         GUI_COLOR(GUI_COLOR_CHAT), pos);
                        (void) plugin_msg_handler_exec (server->name,
                                                        "weechat_pv",
                                                        irc_message);
                    }
                }
            }
            else
            {
                /* unknown CTCP ? */
                pos2 = strchr (pos + 1, '\01');
                if ((pos[0] == '\01') && pos2 && (pos2[1] == '\0'))
                {
                    if (!ignore)
                    {
                        pos++;
                        pos2[0] = '\0';
                        pos2 = strchr (pos, ' ');
                        if (pos2)
                        {
                            pos2[0] = '\0';
                            pos2++;
                            while (pos2[0] == ' ')
                                pos2++;
                            if (!pos2[0])
                                pos2 = NULL;
                        }
                        gui_chat_printf_server (server->buffer,
                                                _("Unknown CTCP %s%s%s "
                                                  "received from %s%s"),
                                                GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                                pos,
                                                GUI_COLOR(GUI_COLOR_CHAT),
                                                GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                                nick);
                        if (pos2)
                            gui_chat_printf (server->buffer, "%s: %s\n",
                                             GUI_COLOR(GUI_COLOR_CHAT),
                                             pos2);
                        else
                            gui_chat_printf (server->buffer, "\n");
                        (void) plugin_msg_handler_exec (server->name,
                                                        "weechat_ctcp",
                                                        irc_message);
                    }
                    return 0;
                }
                else
                {
                    if (!ignore)
                    {
                        if (!ptr_channel)
                        {
                            ptr_channel = irc_channel_new (server,
                                                           IRC_CHANNEL_TYPE_PRIVATE,
                                                           nick, 0);
                            if (!ptr_channel)
                            {
                                gui_chat_printf_error_nolog (server->buffer,
                                                             _("%s cannot "
                                                               "create new "
                                                               "private "
                                                               "window "
                                                               "\"%s\"\n"),
                                                             WEECHAT_ERROR,
                                                             nick);
                                return -1;
                            }
                        }
                        if (!ptr_channel->topic)
                        {
                            ptr_channel->topic = strdup (host2);
                            gui_chat_draw_title (ptr_channel->buffer, 1);
                        }
                        
                        if (highlight || irc_protocol_is_highlight (pos, server->nick))
                        {
                            irc_display_nick (ptr_channel->buffer, NULL, nick,
                                              GUI_MSG_TYPE_NICK | GUI_MSG_TYPE_HIGHLIGHT, 1,
                                              GUI_COLOR(GUI_COLOR_CHAT_HIGHLIGHT), 0);
                            if ((cfg_look_infobar_delay_highlight > 0)
                                && (ptr_channel->buffer != gui_current_window->buffer))
                                gui_infobar_printf (cfg_look_infobar_delay_highlight,
                                                    GUI_COLOR_INFOBAR_HIGHLIGHT,
                                                    _("Private %s> %s"),
                                                    nick, pos);
                            highlight_displayed = 1;
                        }
                        else
                        {
                            irc_display_nick (ptr_channel->buffer, NULL, nick,
                                              GUI_MSG_TYPE_NICK, 1,
                                              GUI_COLOR(GUI_COLOR_CHAT_NICK_OTHER), 0);
                            highlight_displayed = 0;
                        }
                        gui_chat_printf_type (ptr_channel->buffer,
                                              GUI_MSG_TYPE_MSG,
                                              NULL, -1,
                                              "%s%s\n",
                                              GUI_COLOR(GUI_COLOR_CHAT),
                                              pos);
                        (void) plugin_msg_handler_exec (server->name,
                                                        "weechat_pv",
                                                        irc_message);
                        if (highlight_displayed)
                            (void) plugin_msg_handler_exec (server->name,
                                                            "weechat_highlight",
                                                            irc_message);
                    }
                }
            }
        }
        else
        {
            gui_chat_printf_error_nolog (server->buffer,
                                         _("%s cannot parse \"%s\" command\n"),
                                         WEECHAT_ERROR, "privmsg");
            return -1;
        }
    }
    return 0;
}

/*
 * irc_protocol_cmd_quit: 'quit' command received
 */

int
irc_protocol_cmd_quit (t_irc_server *server, char *irc_message, char *host,
                       char *nick, char *arguments, int ignore, int highlight)
{
    char *pos;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) highlight;
    
    /* no host => we can't identify sender of message! */
    if (!host)
    {
        gui_chat_printf_error_nolog (server->buffer,
                                     _("%s \"%s\" command received without "
                                       "host\n"),
                                     WEECHAT_ERROR, "quit");
        return -1;
    }

    if (arguments[0] == ':')
        arguments++;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if ((ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
            || (ptr_channel->type == IRC_CHANNEL_TYPE_DCC_CHAT))
            ptr_nick = NULL;
        else
            ptr_nick = irc_nick_search (ptr_channel, nick);
        
        if (ptr_nick || (strcmp (ptr_channel->name, nick) == 0))
        {
            if (ptr_nick)
                irc_nick_free (ptr_channel, ptr_nick);
            if (!ignore)
            {
                pos = strchr (host, '!');
                gui_chat_printf_quit (ptr_channel->buffer,
                                 _("%s%s %s(%s%s%s)%s has quit"),
                                 GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                 nick,
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                 GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                 (pos) ? pos + 1 : "",
                                 GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                 GUI_COLOR(GUI_COLOR_CHAT));
                gui_chat_printf (ptr_channel->buffer,
                            " %s(%s%s%s)\n",
                            GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                            GUI_COLOR(GUI_COLOR_CHAT),
                            arguments,
                            GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
            }
            gui_nicklist_draw (ptr_channel->buffer, 1, 1);
            gui_status_draw (ptr_channel->buffer, 1);
        }
    }
    
    return 0;
}

/*
 * irc_protocol_cmd_server_mode_reason: command received from server (numeric),
 *                                  format: "mode :reason"
 */

int
irc_protocol_cmd_server_mode_reason (t_irc_server *server, char *irc_message, char *host,
                                     char *nick, char *arguments, int ignore, int highlight)
{
    char *ptr_msg;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
        /* skip nickname if at beginning of server message */
        if (strncmp (server->nick, arguments, strlen (server->nick)) == 0)
        {
            arguments += strlen (server->nick) + 1;
            while (arguments[0] == ' ')
                arguments++;
        }

        ptr_msg = strchr (arguments, ' ');
        if (ptr_msg)
        {
            ptr_msg[0] = '\0';
            ptr_msg++;
            while (ptr_msg[0] == ' ')
                ptr_msg++;
            if (ptr_msg[0] == ':')
                ptr_msg++;
        }
        
        gui_chat_printf_server (server->buffer, "%s%s: %s\n",
                           GUI_COLOR(GUI_COLOR_CHAT), arguments,
                           (ptr_msg) ? ptr_msg : "");
    }
    return 0;
}

/*
 * irc_protocol_cmd_server_msg: command received from server (numeric)
 */

int
irc_protocol_cmd_server_msg (t_irc_server *server, char *irc_message, char *host,
                             char *nick, char *arguments, int ignore, int highlight)
{
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
        while (arguments[0] == ' ')
            arguments++;
        
        /* skip nickname if at beginning of server message */
        if (strncasecmp (server->nick, arguments, strlen (server->nick)) == 0)
        {
            arguments += strlen (server->nick) + 1;
            while (arguments[0] == ' ')
                arguments++;
        }
        
        if (arguments[0] == ':')
            arguments++;
        
        gui_chat_printf_server (server->buffer, "%s%s\n",
                           GUI_COLOR(GUI_COLOR_CHAT), arguments);
    }
    return 0;
}

/*
 * irc_protocol_cmd_topic: 'topic' command received
 */

int
irc_protocol_cmd_topic (t_irc_server *server, char *irc_message, char *host,
                        char *nick, char *arguments, int ignore, int highlight)
{
    char *pos;
    t_irc_channel *ptr_channel;
    t_gui_buffer *buffer;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) highlight;
    
    if (!irc_channel_is_channel (arguments))
    {
        gui_chat_printf_error_nolog (server->buffer,
                                _("%s \"%s\" command received without channel\n"),
                                WEECHAT_ERROR, "topic");
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
    
    ptr_channel = irc_channel_search (server, arguments);
    buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
    
    if (!ignore)
    {
        if (pos)
        {
            gui_chat_printf_info (buffer,
                             _("%s%s%s has changed topic for %s%s%s to:"),
                             GUI_COLOR(GUI_COLOR_CHAT_NICK),
                             nick,
                             GUI_COLOR(GUI_COLOR_CHAT),
                             GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                             arguments,
                             GUI_COLOR(GUI_COLOR_CHAT));
            gui_chat_printf (buffer, " \"%s%s\"\n", pos, GUI_NO_COLOR);
        }
        else
            gui_chat_printf_info (buffer,
                             _("%s%s%s has unset topic for %s%s\n"),
                             GUI_COLOR(GUI_COLOR_CHAT_NICK),
                             nick,
                             GUI_COLOR(GUI_COLOR_CHAT),
                             GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
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
        gui_chat_draw_title (ptr_channel->buffer, 1);
    }
    
    return 0;
}

/*
 * irc_protocol_cmd_wallops: 'wallops' command received
 */

int
irc_protocol_cmd_wallops (t_irc_server *server, char *irc_message, char *host,
                          char *nick, char *arguments, int ignore, int highlight)
{
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) highlight;
    
    if (!ignore)
    {
        if (arguments[0] == ':')
            arguments++;
        gui_chat_printf_server (server->buffer,
                           _("WALLOPS from %s%s%s: %s\n"),
                           GUI_COLOR(GUI_COLOR_CHAT_NICK),
                           nick,
                           GUI_COLOR(GUI_COLOR_CHAT),
                           arguments);
    }
    
    return 0;
}

/*
 * irc_protocol_cmd_001: '001' command (connected to irc server)
 */

int
irc_protocol_cmd_001 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos;
    char **commands, **ptr, *vars_replaced;
    char *away_msg;
    
    pos = strchr (arguments, ' ');
    if (pos)
        pos[0] = '\0';
    if (strcmp (server->nick, arguments) != 0)
    {
        free (server->nick);
        server->nick = strdup (arguments);
    }
    
    irc_protocol_cmd_server_msg (server, irc_message, host, nick, arguments,
                                 ignore, highlight);
    
    /* connection to IRC server is ok! */
    server->is_connected = 1;
    server->lag_next_check = time (NULL) + irc_cfg_irc_lag_check;
    
    /* set away message if user was away (before disconnection for example) */
    if (server->away_message && server->away_message[0])
    {
        away_msg = strdup (server->away_message);
        if (away_msg)
        {
            irc_cmd_away_server (server, away_msg);
            free (away_msg);
        }
    }
    
    /* execute command when connected */
    if (server->command && server->command[0])
    {	
	/* splitting command on ';' which can be escaped with '\;' */ 
	commands = weechat_split_multi_command (server->command, ';');
	if (commands)
	{
	    for (ptr = commands; *ptr; ptr++)
            {
                vars_replaced = irc_protocol_replace_vars (server, NULL, *ptr);
                protocol_input_data (gui_window_search_by_buffer (server->buffer),
                                     (vars_replaced) ? vars_replaced : *ptr, 0);
                if (vars_replaced)
                    free (vars_replaced);
            }
	    weechat_free_multi_command (commands);
	}
	
	if (server->command_delay > 0)
            server->command_time = time (NULL) + 1;
        else
            irc_server_autojoin_channels (server);
    }
    else
        irc_server_autojoin_channels (server);
    
    gui_status_draw (server->buffer, 1);
    gui_input_draw (server->buffer, 1);
    
    return 0;
}

/*
 * irc_protocol_cmd_005: '005' command (some infos from server)
 */

int
irc_protocol_cmd_005 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos, *pos2;
    
    irc_protocol_cmd_server_msg (server, irc_message, host, nick, arguments,
                                 ignore, highlight);
    
    pos = strstr (arguments, "PREFIX=");
    if (pos)
    {
        pos += 7;
        if (pos[0] == '(')
        {
            pos2 = strchr (pos, ')');
            if (!pos2)
                return 0;
            pos = pos2 + 1;
        }
        pos2 = strchr (pos, ' ');
        if (pos2)
            pos2[0] = '\0';
        if (server->prefix)
            free (server->prefix);
        server->prefix = strdup (pos);
        if (pos2)
            pos2[0] = ' ';
    }
    
    return 0;
}

/*
 * irc_protocol_cmd_221: '221' command (user mode string)
 */

int
irc_protocol_cmd_221 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_mode;
    
    /* make C compiler happy */
    (void) server;
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    pos_mode = strchr (arguments, ' ');
    if (pos_mode)
    {
        pos_mode[0] = '\0';
        pos_mode++;
        while (pos_mode[0] == ' ')
            pos_mode++;
        
        if (!ignore)
        {
            gui_chat_printf_server (server->buffer,
                               _("User mode for %s%s%s is %s[%s%s%s]\n"),
                               GUI_COLOR(GUI_COLOR_CHAT_NICK),
                               arguments,
                               GUI_COLOR(GUI_COLOR_CHAT),
                               GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                               GUI_COLOR(GUI_COLOR_CHAT),
                               pos_mode,
                               GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
        }
    }
    else
    {
        gui_chat_printf_error_nolog (server->buffer,
                                _("%s cannot parse \"%s\" command\n"),
                                WEECHAT_ERROR, "221");
        return -1;
    }
    
    return 0;
}

/*
 * irc_protocol_cmd_301: '301' command (away message)
 */

int
irc_protocol_cmd_301 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_nick, *pos_message;
    t_irc_channel *ptr_channel;
    t_gui_buffer *ptr_buffer;
    
    /* make C compiler happy */
    (void) server;
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
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
            
            if (!ignore)
            {
                /* look for private buffer to display message */
                ptr_channel = irc_channel_search (server, pos_nick);
                if (!irc_cfg_irc_show_away_once || !ptr_channel ||
                    !(ptr_channel->away_message) ||
                    (strcmp (ptr_channel->away_message, pos_message) != 0))
                {
                    ptr_buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
                    gui_chat_printf_info (ptr_buffer,
                                     _("%s%s%s is away: %s\n"),
                                     GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                     pos_nick,
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     pos_message);
                    if (ptr_channel)
                    {
                        if (ptr_channel->away_message)
                            free (ptr_channel->away_message);
                        ptr_channel->away_message = strdup (pos_message);
                    }
                }
            }
        }
    }
    return 0;
}

/*
 * irc_protocol_cmd_302: '302' command (userhost)
 */

int
irc_protocol_cmd_302 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_host, *ptr_next;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
        arguments = strchr (arguments, ' ');
        if (arguments)
        {
            while (arguments[0] == ' ')
                arguments++;
            if (arguments[0] == ':')
                arguments++;
            while (arguments)
            {
                pos_host = strchr (arguments, '=');
                if (pos_host)
                {
                    pos_host[0] = '\0';
                    pos_host++;
                    
                    ptr_next = strchr (pos_host, ' ');
                    if (ptr_next)
                    {
                        ptr_next[0] = '\0';
                        ptr_next++;
                        while (ptr_next[0] == ' ')
                            ptr_next++;
                    }
                    
                    gui_chat_printf_server (server->buffer,
                                       "%s%s%s=%s%s\n",
                                       GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                       arguments,
                                       GUI_COLOR(GUI_COLOR_CHAT),
                                       GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                       pos_host);
                }
                else
                    ptr_next = NULL;
                arguments = ptr_next;
                if (arguments && !arguments[0])
                    arguments = NULL;
            }
        }
    }
    return 0;
}

/*
 * irc_protocol_cmd_303: '303' command (ison)
 */

int
irc_protocol_cmd_303 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *ptr_next;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
        gui_chat_printf_server (server->buffer, _("Users online: "));
        
        arguments = strchr (arguments, ' ');
        if (arguments)
        {
            while (arguments[0] == ' ')
                arguments++;
            if (arguments[0] == ':')
                arguments++;
            while (arguments)
            {
                ptr_next = strchr (arguments, ' ');
                if (ptr_next)
                {
                    ptr_next[0] = '\0';
                    ptr_next++;
                    while (ptr_next[0] == ' ')
                        ptr_next++;
                }
                gui_chat_printf (server->buffer, "%s%s ",
                            GUI_COLOR(GUI_COLOR_CHAT_NICK),
                            arguments);
                arguments = ptr_next;
                if (arguments && !arguments[0])
                    arguments = NULL;
            }
        }
        gui_chat_printf (server->buffer, "\n");
    }
    return 0;
}

/*
 * irc_protocol_cmd_305: '305' command (unaway)
 */

int
irc_protocol_cmd_305 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    t_gui_window *ptr_window;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
        arguments = strchr (arguments, ' ');
        if (arguments)
        {
            while (arguments[0] == ' ')
                arguments++;
            if (arguments[0] == ':')
                arguments++;
            gui_chat_printf_server (server->buffer, "%s\n", arguments);
        }
    }
    server->is_away = 0;
    server->away_time = 0;
    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if ((ptr_window->buffer->protocol == irc_protocol)
            && (IRC_BUFFER_SERVER(ptr_window->buffer) == server))
            gui_status_draw (ptr_window->buffer, 1);
    }
    return 0;
}

/*
 * irc_protocol_cmd_306: '306' command (now away)
 */

int
irc_protocol_cmd_306 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    t_gui_window *ptr_window;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
        arguments = strchr (arguments, ' ');
        if (arguments)
        {
            while (arguments[0] == ' ')
                arguments++;
            if (arguments[0] == ':')
                arguments++;
            gui_chat_printf_server (server->buffer, "%s\n", arguments);
        }
    }
    server->is_away = 1;
    server->away_time = time (NULL);
    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if (ptr_window->buffer->protocol == irc_protocol)
        {
            if (IRC_BUFFER_SERVER(ptr_window->buffer) == server)
            {
                gui_status_draw (ptr_window->buffer, 1);
                ptr_window->buffer->last_read_line =
                    ptr_window->buffer->last_line;
            }
        }
    }
    return 0;
}

/*
 * irc_protocol_cmd_whois_nick_msg: a whois command with nick and message
 */

int
irc_protocol_cmd_whois_nick_msg (t_irc_server *server, char *irc_message, char *host,
                                 char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_nick, *pos_msg;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
        pos_nick = strchr (arguments, ' ');
        if (pos_nick)
        {
            while (pos_nick[0] == ' ')
                pos_nick++;
            pos_msg = strchr (pos_nick, ' ');
            if (pos_msg)
            {
                pos_msg[0] = '\0';
                pos_msg++;
                while (pos_msg[0] == ' ')
                    pos_msg++;
                if (pos_msg[0] == ':')
                    pos_msg++;
                
                gui_chat_printf_server (server->buffer, "%s[%s%s%s] %s%s\n",
                                   GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                   GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                   pos_nick,
                                   GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                   GUI_COLOR(GUI_COLOR_CHAT),
                                   pos_msg);
            }
        }
    }
    return 0;
}

/*
 * irc_protocol_cmd_310: '310' command (whois, help mode)
 */

int
irc_protocol_cmd_310 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_nick;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
        pos_nick = strchr (arguments, ' ');
        if (pos_nick)
        {
            while (pos_nick[0] == ' ')
                pos_nick++;
            
            gui_chat_printf_server (server->buffer,
                               _("%s[%s%s%s]%s help mode (+h)\n"),
                               GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                               GUI_COLOR(GUI_COLOR_CHAT_NICK),
                               pos_nick,
                               GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                               GUI_COLOR(GUI_COLOR_CHAT));
        }
    }
    return 0;
}

/*
 * irc_protocol_cmd_311: '311' command (whois, user)
 */

int
irc_protocol_cmd_311 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_nick, *pos_user, *pos_host, *pos_realname;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
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
                        
                        gui_chat_printf_server (server->buffer,
                                           "%s[%s%s%s] (%s%s@%s%s)%s: %s\n",
                                           GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                           GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                           pos_nick,
                                           GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                           GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                           pos_user,
                                           pos_host,
                                           GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                           GUI_COLOR(GUI_COLOR_CHAT),
                                           pos_realname);
                    }
                }
            }
        }
    }
    return 0;
}

/*
 * irc_protocol_cmd_312: '312' command (whois, server)
 */

int
irc_protocol_cmd_312 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_nick, *pos_server, *pos_serverinfo;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
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
                    
                    gui_chat_printf_server (server->buffer,
                                       "%s[%s%s%s] %s%s %s(%s%s%s)\n",
                                       GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                       GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                       pos_nick,
                                       GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                       GUI_COLOR(GUI_COLOR_CHAT),
                                       pos_server,
                                       GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                       GUI_COLOR(GUI_COLOR_CHAT),
                                       pos_serverinfo,
                                       GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
                }
            }
        }
    }
    return 0;
}

/*
 * irc_protocol_cmd_314: '314' command (whowas)
 */

int
irc_protocol_cmd_314 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_nick, *pos_user, *pos_host, *pos_realname;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;

    if (!ignore)
    {
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
                        pos_realname = strchr (pos_realname, ' ');
                        if (pos_realname)
                        {
                            pos_realname[0] = '\0';
                            pos_realname++;
                            while (pos_realname[0] == ' ')
                                pos_realname++;
                            if (pos_realname[0] == ':')
                                pos_realname++;
                            
                            gui_chat_printf_server (server->buffer,
                                               _("%s%s %s(%s%s@%s%s)%s was %s\n"),
                                               GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                               pos_nick,
                                               GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                               GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                               pos_user,
                                               pos_host,
                                               GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                               GUI_COLOR(GUI_COLOR_CHAT),
                                               pos_realname);
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/*
 * irc_protocol_cmd_315: '315' command (end of /who)
 */

int
irc_protocol_cmd_315 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos;
    t_irc_channel *ptr_channel;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    /* skip nickname if at beginning of server message */
    if (strncmp (server->nick, arguments, strlen (server->nick)) == 0)
    {
        arguments += strlen (server->nick) + 1;
        while (arguments[0] == ' ')
            arguments++;
    }
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        ptr_channel = irc_channel_search (server, arguments);
        if (ptr_channel && (ptr_channel->checking_away > 0))
        {
            ptr_channel->checking_away--;
            return 0;
        }
        if (!ignore)
        {
            gui_chat_printf_server (server->buffer,
                               "%s%s %s%s\n",
                               GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                               arguments,
                               GUI_COLOR(GUI_COLOR_CHAT),
                               pos);
        }
    }
    else
    {
        if (!ignore)
            gui_chat_printf_server (server->buffer, "%s\n", arguments);
    }
    return 0;
}

/*
 * irc_protocol_cmd_317: '317' command (whois, idle)
 */

int
irc_protocol_cmd_317 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_nick, *pos_idle, *pos_signon, *pos_message;
    int idle_time, day, hour, min, sec;
    time_t datetime;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
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
                        
                        gui_chat_printf_server (server->buffer,
                                           _("%s[%s%s%s]%s idle: "),
                                           GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                           GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                           pos_nick,
                                           GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                           GUI_COLOR(GUI_COLOR_CHAT));
                        
                        if (day > 0)
                            gui_chat_printf (server->buffer, "%s%d %s%s, ",
                                        GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                        day,
                                        GUI_COLOR(GUI_COLOR_CHAT),
                                        (day > 1) ? _("days") : _("day"));
                        
                        datetime = (time_t)(atol (pos_signon));
                        gui_chat_printf (server->buffer,
                                    _("%s%02d %s%s %s%02d %s%s %s%02d %s%s, signon at: %s%s"),
                                    GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                    hour,
                                    GUI_COLOR(GUI_COLOR_CHAT),
                                    (hour > 1) ? _("hours") : _("hour"),
                                    GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                    min,
                                    GUI_COLOR(GUI_COLOR_CHAT),
                                    (min > 1) ? _("minutes") : _("minute"),
                                    GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                    sec,
                                    GUI_COLOR(GUI_COLOR_CHAT),
                                    (sec > 1) ? _("seconds") : _("second"),
                                    GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                    ctime (&datetime));
                    }
                }
            }
        }
    }
    return 0;
}

/*
 * irc_protocol_cmd_319: '319' command (whois, channels)
 */

int
irc_protocol_cmd_319 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_nick, *pos_channel, *pos;
    int color;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
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
                
                gui_chat_printf_server (server->buffer,
                                   _("%s[%s%s%s]%s Channels: "),
                                   GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                   GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                   pos_nick,
                                   GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                   GUI_COLOR(GUI_COLOR_CHAT));
                while (pos_channel && pos_channel[0])
                {
                    if (irc_mode_nick_prefix_allowed (server, pos_channel[0]))
                    {
                        switch (pos_channel[0])
                        {
                            case '@': /* op */
                                color = GUI_COLOR_NICKLIST_PREFIX1;
                                break;
                            case '~': /* channel owner */
                                color = GUI_COLOR_NICKLIST_PREFIX1;
                                break;
                            case '&': /* channel admin */
                                color = GUI_COLOR_NICKLIST_PREFIX1;
                                break;
                            case '!': /* channel admin (2) */
                                color = GUI_COLOR_NICKLIST_PREFIX1;
                                break;
                            case '%': /* half-op */
                                color = GUI_COLOR_NICKLIST_PREFIX2;
                                break;
                            case '+': /* voice */
                                color = GUI_COLOR_NICKLIST_PREFIX3;
                                break;
                            case '-': /* channel user */
                                color = GUI_COLOR_NICKLIST_PREFIX4;
                                break;
                            default:
                                color = GUI_COLOR_CHAT;
                                break;
                        }
                        gui_chat_printf (server->buffer, "%s%c",
                                    GUI_COLOR(color), pos_channel[0]);
                        pos_channel++;
                    }
                    
                    pos = strchr (pos_channel, ' ');
                    if (pos)
                    {
                        pos[0] = '\0';
                        pos++;
                        while (pos[0] == ' ')
                            pos++;
                    }
                    gui_chat_printf (server->buffer, "%s%s%s",
                                GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                pos_channel,
                                (pos && pos[0]) ? " " : "\n");
                    pos_channel = pos;
                }
            }
        }
    }
    return 0;
}

/*
 * irc_protocol_cmd_321: '321' command (/list start)
 */

int
irc_protocol_cmd_321 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
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
        
        gui_chat_printf_server (server->buffer, "%s\n", pos);
    }
    return 0;
}

/*
 * irc_protocol_cmd_322: '322' command (channel for /list)
 */

int
irc_protocol_cmd_322 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
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
        
	if (server->cmd_list_regexp)
	{
	    if (regexec (server->cmd_list_regexp, pos, 0, NULL, 0) == 0)
		gui_chat_printf_server (server->buffer, "%s\n", pos);
	}
	else
	    gui_chat_printf_server (server->buffer, "%s\n", pos);
    }
    return 0;
}

/*
 * irc_protocol_cmd_323: '323' command (/list end)
 */

int
irc_protocol_cmd_323 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
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
        
        gui_chat_printf_server (server->buffer, "%s\n", pos);
    }
    return 0;
}

/*
 * irc_protocol_cmd_324: '324' command (channel mode)
 */

int
irc_protocol_cmd_324 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_channel, *pos_modes, *pos;
    t_irc_channel *ptr_channel;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) ignore;
    (void) highlight;
    
    pos_channel = strchr (arguments, ' ');
    if (pos_channel)
    {
        pos_channel[0] = '\0';
        pos_channel++;
        while (pos_channel[0] == ' ')
            pos_channel++;
        
        pos_modes = strchr (pos_channel, ' ');
        if (pos_modes)
        {
            pos_modes[0] = '\0';
            pos_modes++;
            while (pos_modes[0] == ' ')
                pos_modes++;

            /* remove spaces after modes */
            pos = pos_modes + strlen (pos_modes) - 1;
            while ((pos >= pos_modes) && (pos[0] == ' '))
            {
                pos[0] = '\0';
                pos--;
            }

            /* search channel */
            ptr_channel = irc_channel_search (server, pos_channel);
            if (ptr_channel)
            {
                if (pos_modes[0])
                {
                    if (ptr_channel->modes)
                        ptr_channel->modes = (char *) realloc (ptr_channel->modes,
                                                               strlen (pos_modes) + 1);
                    else
                        ptr_channel->modes = (char *) malloc (strlen (pos_modes) + 1);
                    strcpy (ptr_channel->modes, pos_modes);
                    irc_mode_channel_set (server, ptr_channel, pos_modes);
                }
                else
                {
                    if (ptr_channel->modes)
                    {
                        free (ptr_channel->modes);
                        ptr_channel->modes = NULL;
                    }
                }
                gui_status_draw (ptr_channel->buffer, 1);
            }
        }
    }
    return 0;
}

/*
 * irc_protocol_cmd_327: '327' command (whois, host)
 */

int
irc_protocol_cmd_327 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_nick, *pos_host1, *pos_host2, *pos_other;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
        pos_nick = strchr (arguments, ' ');
        if (pos_nick)
        {
            while (pos_nick[0] == ' ')
                pos_nick++;
            pos_host1 = strchr (pos_nick, ' ');
            if (pos_host1)
            {
                pos_host1[0] = '\0';
                pos_host1++;
                while (pos_host1[0] == ' ')
                    pos_host1++;
                pos_host2 = strchr (pos_host1, ' ');
                if (pos_host2)
                {
                    pos_host2[0] = '\0';
                    pos_host2++;
                    while (pos_host2[0] == ' ')
                        pos_host2++;

                    pos_other = strchr (pos_host2, ' ');
                    if (pos_other)
                    {
                        pos_other[0] = '\0';
                        pos_other++;
                        while (pos_other[0] == ' ')
                            pos_other++;
                    }
                    
                    gui_chat_printf_server (server->buffer,
                                       "%s[%s%s%s] %s%s %s %s%s%s%s%s%s\n",
                                       GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                       GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                       pos_nick,
                                       GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                       GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                       pos_host1,
                                       pos_host2,
                                       GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                       (pos_other) ? "(" : "",
                                       GUI_COLOR(GUI_COLOR_CHAT),
                                       (pos_other) ? pos_other : "",
                                       GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                       (pos_other) ? ")" : "");
                }
            }
        }
    }
    return 0;
}

/*
 * irc_protocol_cmd_329: '329' command received (channel creation date)
 */

int
irc_protocol_cmd_329 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_channel, *pos_date;
    t_irc_channel *ptr_channel;
    time_t datetime;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    pos_channel = strchr (arguments, ' ');
    if (pos_channel)
    {
        while (pos_channel[0] == ' ')
            pos_channel++;
        pos_date = strchr (pos_channel, ' ');
        if (pos_date)
        {
            pos_date[0] = '\0';
            pos_date++;
            while (pos_date[0] == ' ')
                pos_date++;
            
            ptr_channel = irc_channel_search (server, pos_channel);
            if (!ptr_channel)
            {
                gui_chat_printf_error_nolog (server->buffer,
                                        _("%s channel \"%s\" not found for "
                                          "\"%s\" command\n"),
                                        WEECHAT_ERROR, pos_channel, "329");
                return -1;
            }
            
            if (!ignore && (ptr_channel->display_creation_date))
            {
                datetime = (time_t)(atol (pos_date));
                gui_chat_printf_info (ptr_channel->buffer,
                                 _("Channel created on %s"),
                                 ctime (&datetime));
            }
            ptr_channel->display_creation_date = 0;
        }
        else
        {
            gui_chat_printf_error_nolog (server->buffer,
                                    _("%s cannot identify date/time for "
                                      "\"%s\" command\n"),
                                    WEECHAT_ERROR, "329");
            return -1;
        }
    }
    else
    {
        gui_chat_printf_error_nolog (server->buffer,
                                _("%s cannot identify channel for "
                                  "\"%s\" command\n"),
                                WEECHAT_ERROR, "329");
        return -1;
    }
    return 0;
}

/*
 * irc_protocol_cmd_331: '331' command received (no topic for channel)
 */

int
irc_protocol_cmd_331 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_channel, *pos;
    t_irc_channel *ptr_channel;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) server;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
        pos_channel = strchr (arguments, ' ');
        if (pos_channel)
        {
            pos_channel++;
            while (pos_channel[0] == ' ')
                pos_channel++;
            pos = strchr (pos_channel, ' ');
            if (pos)
                pos[0] = '\0';
        }
        else
        {
            gui_chat_printf_error_nolog (server->buffer,
                                    _("%s channel \"%s\" not found for "
                                      "\"%s\" command\n"),
                                    WEECHAT_ERROR, "", "331");
            return -1;
        }
        
        ptr_channel = irc_channel_search (server, pos_channel);
        if (!ignore)
            gui_chat_printf_info ((ptr_channel) ? ptr_channel->buffer : NULL,
                             _("No topic set for %s%s\n"),
                             GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                             pos_channel);
    }
    return 0;
}

/*
 * irc_protocol_cmd_332: '332' command received (topic of channel)
 */

int
irc_protocol_cmd_332 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos, *pos2;
    t_irc_channel *ptr_channel;
    t_gui_buffer *ptr_buffer;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        while (pos[0] == ' ')
            pos++;
        pos2 = strchr (pos, ' ');
        if (pos2)
        {
            pos2[0] = '\0';
            ptr_channel = irc_channel_search (server, pos);
            ptr_buffer = (ptr_channel) ?
                ptr_channel->buffer : server->buffer;
            pos2++;
            while (pos2[0] == ' ')
                pos2++;
            if (pos2[0] == ':')
                pos2++;
            if (ptr_channel)
            {
                if (ptr_channel->topic)
                    free (ptr_channel->topic);
                ptr_channel->topic = strdup (pos2);
            }
            
            if (!ignore)
            {
                gui_chat_printf_info (ptr_buffer, _("Topic for %s%s%s is: "),
                                 GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                 pos,
                                 GUI_COLOR(GUI_COLOR_CHAT));
                gui_chat_printf (ptr_buffer, "\"%s%s\"\n", pos2, GUI_NO_COLOR);
            }
            
            if (ptr_channel)
                gui_chat_draw_title (ptr_buffer, 1);
        }
    }
    else
    {
        gui_chat_printf_error_nolog (server->buffer,
                                _("%s cannot identify channel for \"%s\" "
                                  "command\n"),
                                WEECHAT_ERROR, "332");
        return -1;
    }
    return 0;
}

/*
 * irc_protocol_cmd_333: '333' command received (infos about topic (nick / date))
 */

int
irc_protocol_cmd_333 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_channel, *pos_nick, *pos_date;
    t_irc_channel *ptr_channel;
    t_gui_buffer *ptr_buffer;
    time_t datetime;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
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
                
                ptr_channel = irc_channel_search (server, pos_channel);
                ptr_buffer = (ptr_channel) ?
                    ptr_channel->buffer : server->buffer;
                
                if (!ignore)
                {
                    datetime = (time_t)(atol (pos_date));
                    gui_chat_printf_info (ptr_buffer,
                                     _("Topic set by %s%s%s, %s"),
                                     GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                     pos_nick,
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     ctime (&datetime));
                }
            }
            else
                return 0;
        }
        else
            return 0;
    }
    else
    {
        gui_chat_printf_error_nolog (server->buffer,
                                _("%s cannot identify channel for \"%s\" "
                                  "command\n"),
                                WEECHAT_ERROR, "333");
        return -1;
    }
    return 0;
}

/*
 * irc_protocol_cmd_338: '338' command (whois, host)
 */

int
irc_protocol_cmd_338 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_nick, *pos_host, *pos_message;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
        pos_nick = strchr (arguments, ' ');
        if (pos_nick)
        {
            while (pos_nick[0] == ' ')
                pos_nick++;
            pos_host = strchr (pos_nick, ' ');
            if (pos_host)
            {
                pos_host[0] = '\0';
                pos_host++;
                while (pos_host[0] == ' ')
                    pos_host++;
                pos_message = strchr (pos_host, ' ');
                if (pos_message)
                {
                    pos_message[0] = '\0';
                    pos_message++;
                    while (pos_message[0] == ' ')
                        pos_message++;
                    if (pos_message[0] == ':')
                        pos_message++;
                    
                    gui_chat_printf_server (server->buffer,
                                       "%s[%s%s%s] %s%s %s%s %s%s\n",
                                       GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                       GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                       pos_nick,
                                       GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                       GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                       pos_nick,
                                       GUI_COLOR(GUI_COLOR_CHAT),
                                       pos_message,
                                       GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                       pos_host);
                }
            }
        }
    }
    return 0;
}

/*
 * irc_protocol_cmd_341: '341' command received (inviting)
 */

int
irc_protocol_cmd_341 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_nick, *pos_channel;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        pos_nick[0] = '\0';
        pos_nick++;
        while (pos_nick[0] == ' ')
            pos_nick++;
        
        pos_channel = strchr (pos_nick, ' ');
        if (pos_channel)
        {
            if (!ignore)
            {
                pos_channel[0] = '\0';
                pos_channel++;
                while (pos_channel[0] == ' ')
                    pos_channel++;
                if (pos_channel[0] == ':')
                    pos_channel++;
                
                gui_chat_printf_server (server->buffer,
                                   _("%s%s%s has invited %s%s%s on %s%s\n"),
                                   GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                   arguments,
                                   GUI_COLOR(GUI_COLOR_CHAT),
                                   GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                   pos_nick,
                                   GUI_COLOR(GUI_COLOR_CHAT),
                                   GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                   pos_channel);
                gui_status_draw (gui_current_window->buffer, 1);
            }
        }
        else
        {
            gui_chat_printf_error_nolog (server->buffer,
                                    _("%s cannot identify channel for \"%s\" "
                                      "command\n"),
                                    WEECHAT_ERROR, "341");
            return -1;
        }
    }
    else
    {
        gui_chat_printf_error_nolog (server->buffer,
                                _("%s cannot identify nickname for \"%s\" "
                                  "command\n"),
                                WEECHAT_ERROR, "341");
        return -1;
    }
    return 0;
}

/*
 * irc_protocol_cmd_344: '344' command (channel reop)
 */

int
irc_protocol_cmd_344 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_channel, *pos_host;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;

    if (!ignore)
    {
        pos_channel = strchr (arguments, ' ');
        if (pos_channel)
        {
            while (pos_channel[0] == ' ')
                pos_channel++;
            pos_host = strchr (pos_channel, ' ');
            if (pos_host)
            {
                pos_host[0] = '\0';
                pos_host++;
                while (pos_host[0] == ' ')
                    pos_host++;
                
                gui_chat_printf_server (server->buffer,
                                   _("Channel reop %s%s%s: %s%s\n"),
                                   GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                   pos_channel,
                                   GUI_COLOR(GUI_COLOR_CHAT),
                                   GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                   pos_host);
            }
        }
    }
    return 0;
}

/*
 * irc_protocol_cmd_345: '345' command (end of channel reop)
 */

int
irc_protocol_cmd_345 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    /* skip nickname if at beginning of server message */
    if (strncmp (server->nick, arguments, strlen (server->nick)) == 0)
    {
        arguments += strlen (server->nick) + 1;
        while (arguments[0] == ' ')
            arguments++;
    }
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        if (!ignore)
            gui_chat_printf_server (server->buffer,
                               "%s%s %s%s\n",
                               GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                               arguments,
                               GUI_COLOR(GUI_COLOR_CHAT),
                               pos);
    }
    else
    {
        if (!ignore)
            gui_chat_printf_server (server->buffer, "%s\n", arguments);
    }
    return 0;
}

/*
 * irc_protocol_cmd_348: '348' command received (channel exception list)
 */

int
irc_protocol_cmd_348 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_channel, *pos_exception, *pos_user, *pos_date, *pos;
    t_irc_channel *ptr_channel;
    t_gui_buffer *buffer;
    time_t datetime;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    /* look for channel */
    pos_channel = strchr (arguments, ' ');
    if (!pos_channel)
    {
        gui_chat_printf_error_nolog (server->buffer,
                                _("%s cannot parse \"%s\" command\n"),
                                WEECHAT_ERROR, "348");
        return -1;
    }
    pos_channel[0] = '\0';
    pos_channel++;
    while (pos_channel[0] == ' ')
        pos_channel++;
    
    /* look for exception mask */
    pos_exception = strchr (pos_channel, ' ');
    if (!pos_exception)
    {
        gui_chat_printf_error_nolog (server->buffer,
                                _("%s cannot parse \"%s\" command\n"),
                                WEECHAT_ERROR, "348");
        return -1;
    }
    pos_exception[0] = '\0';
    pos_exception++;
    while (pos_exception[0] == ' ')
        pos_exception++;
    
    /* look for user who set exception */
    pos_user = strchr (pos_exception, ' ');
    if (pos_user)
    {
        pos_user[0] = '\0';
        pos_user++;
        while (pos_user[0] == ' ')
            pos_user++;
        
        /* look for date/time */
        pos_date = strchr (pos_user, ' ');
        if (pos_date)
        {
            pos_date[0] = '\0';
            pos_date++;
            while (pos_date[0] == ' ')
                pos_date++;
        }
    }
    else
        pos_date = NULL;
    
    ptr_channel = irc_channel_search (server, pos_channel);
    buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
    
    if (!ignore)
    {
        gui_chat_printf_info (buffer,
                         _("%s[%s%s%s]%s exception %s%s%s"),
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                         GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                         pos_channel,
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                         GUI_COLOR(GUI_COLOR_CHAT),
                         GUI_COLOR(GUI_COLOR_CHAT_HOST),
                         pos_exception,
                         GUI_COLOR(GUI_COLOR_CHAT));
        if (pos_user)
        {
            pos = strchr (pos_user, '!');
            if (pos)
            {
                pos[0] = '\0';
                gui_chat_printf (buffer,
                            _(" by %s%s %s(%s%s%s)"),
                            GUI_COLOR(GUI_COLOR_CHAT_NICK),
                            pos_user,
                            GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                            GUI_COLOR(GUI_COLOR_CHAT_HOST),
                            pos + 1,
                            GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
            }
            else
                gui_chat_printf (buffer,
                            _(" by %s%s"),
                            GUI_COLOR(GUI_COLOR_CHAT_NICK),
                            pos_user);
        }
        if (pos_date)
        {
            datetime = (time_t)(atol (pos_date));
            gui_chat_printf_nolog (buffer, NULL, -1,
                              "%s, %s",
                              GUI_COLOR(GUI_COLOR_CHAT),
                              ctime (&datetime));
        }
        else
            gui_chat_printf_nolog (buffer, NULL, -1, "\n");
    }
    return 0;
}

/*
 * irc_protocol_cmd_349: '349' command received (end of channel exception list)
 */

int
irc_protocol_cmd_349 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_channel, *pos_msg;
    t_irc_channel *ptr_channel;
    t_gui_buffer *buffer;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    pos_channel = strchr (arguments, ' ');
    if (!pos_channel)
    {
        gui_chat_printf_error_nolog (server->buffer,
                                _("%s cannot parse \"%s\" command\n"),
                                WEECHAT_ERROR, "349");
        return -1;
    }
    pos_channel[0] = '\0';
    pos_channel++;
    while (pos_channel[0] == ' ')
        pos_channel++;
    
    pos_msg = strchr (pos_channel, ' ');
    if (!pos_msg)
    {
        gui_chat_printf_error_nolog (server->buffer,
                                _("%s cannot parse \"%s\" command\n"),
                                WEECHAT_ERROR, "349");
        return -1;
    }
    pos_msg[0] = '\0';
    pos_msg++;
    while (pos_msg[0] == ' ')
        pos_msg++;
    if (pos_msg[0] == ':')
        pos_msg++;
    
    ptr_channel = irc_channel_search (server, pos_channel);
    buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
    
    if (!ignore)
    {
        gui_chat_printf_info (buffer,
                         "%s[%s%s%s] %s%s\n",
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                         GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                         pos_channel,
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                         GUI_COLOR(GUI_COLOR_CHAT),
                         pos_msg);
    }
    return 0;
}

/*
 * irc_protocol_cmd_351: '351' command received (server version)
 */

int
irc_protocol_cmd_351 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos, *pos2;
    
    /* make C compiler happy */
    (void) server;
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
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
    
    if (!ignore)
    {
        if (pos2)
            gui_chat_printf_server (server->buffer, "%s %s\n", pos, pos2);
        else
            gui_chat_printf_server (server->buffer, "%s\n", pos);
    }
    return 0;
}

/*
 * irc_protocol_cmd_352: '352' command (who)
 */

int
irc_protocol_cmd_352 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_channel, *pos_user, *pos_host, *pos_server, *pos_nick;
    char *pos_attr, *pos_hopcount, *pos_realname;
    int length;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    pos_channel = strchr (arguments, ' ');
    if (pos_channel)
    {
        while (pos_channel[0] == ' ')
            pos_channel++;
        pos_user = strchr (pos_channel, ' ');
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
                pos_server = strchr (pos_host, ' ');
                if (pos_server)
                {
                    pos_server[0] = '\0';
                    pos_server++;
                    while (pos_server[0] == ' ')
                        pos_server++;
                    pos_nick = strchr (pos_server, ' ');
                    if (pos_nick)
                    {
                        pos_nick[0] = '\0';
                        pos_nick++;
                        while (pos_nick[0] == ' ')
                            pos_nick++;
                        pos_attr = strchr (pos_nick, ' ');
                        if (pos_attr)
                        {
                            pos_attr[0] = '\0';
                            pos_attr++;
                            while (pos_attr[0] == ' ')
                                pos_attr++;
                            pos_hopcount = strchr (pos_attr, ' ');
                            if (pos_hopcount)
                            {
                                pos_hopcount[0] = '\0';
                                pos_hopcount++;
                                while (pos_hopcount[0] == ' ')
                                    pos_hopcount++;
                                if (pos_hopcount[0] == ':')
                                    pos_hopcount++;
                                pos_realname = strchr (pos_hopcount, ' ');
                                if (pos_realname)
                                {
                                    pos_realname[0] = '\0';
                                    pos_realname++;
                                    while (pos_realname[0] == ' ')
                                        pos_realname++;
                                    
                                    ptr_channel = irc_channel_search (server, pos_channel);
                                    if (ptr_channel && (ptr_channel->checking_away > 0))
                                    {
                                        ptr_nick = irc_nick_search (ptr_channel, pos_nick);
                                        if (ptr_nick)
                                        {
                                            if (ptr_nick->host)
                                                free (ptr_nick->host);
                                            length = strlen (pos_user) + 1 + strlen (pos_host) + 1;
                                            ptr_nick->host = (char *) malloc (length);
                                            if (ptr_nick->host)
                                                snprintf (ptr_nick->host, length, "%s@%s", pos_user, pos_host);
                                            irc_nick_set_away (ptr_channel, ptr_nick,
                                                               (pos_attr[0] == 'G') ? 1 : 0);
                                        }
                                        return 0;
                                    }
                                    
                                    if (!ignore)
                                    {
                                        gui_chat_printf_server (server->buffer,
                                                           _("%s%s%s on %s%s%s "
                                                             "%s %s %s%s@%s "
                                                             "%s(%s%s%s)\n"),
                                                           GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                                           pos_nick,
                                                           GUI_COLOR(GUI_COLOR_CHAT),
                                                           GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                                           pos_channel,
                                                           GUI_COLOR(GUI_COLOR_CHAT),
                                                           pos_attr,
                                                           pos_hopcount,
                                                           GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                                           pos_user,
                                                           pos_host,
                                                           GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                                           GUI_COLOR(GUI_COLOR_CHAT),
                                                           pos_realname,
                                                           GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/*
 * irc_protocol_cmd_353: '353' command received (list of users on a channel)
 */

int
irc_protocol_cmd_353 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos, *pos_nick;
    int is_chanowner, is_chanadmin, is_chanadmin2, is_op, is_halfop;
    int has_voice, is_chanuser;
    int prefix_found, color;
    t_irc_channel *ptr_channel;
    t_gui_buffer *ptr_buffer;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
        
    pos = strstr (arguments, " = ");
    if (pos)
        arguments = pos + 3;
    else
    {
        pos = strstr (arguments, " * ");
        if (pos)
            arguments = pos + 3;
        else
        {
            pos = strstr (arguments, " @ ");
            if (pos)
                arguments = pos + 3;
        }
    }
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        
        ptr_channel = irc_channel_search (server, arguments);
        if (ptr_channel)
            ptr_buffer = ptr_channel->buffer;
        else
            ptr_buffer = server->buffer;
        
        pos++;
        while (pos[0] == ' ')
            pos++;
        if (pos[0] != ':')
        {
            gui_chat_printf_error_nolog (server->buffer,
                                    _("%s cannot parse \"%s\" command\n"),
                                    WEECHAT_ERROR, "353");
            return -1;
        }
        
        /* channel is not joined => display users on server buffer */
        if (!ignore && !ptr_channel)
        {
            /* display users on channel */
            gui_chat_printf_server (ptr_buffer,
                               _("Nicks %s%s%s: %s["),
                               GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                               arguments,
                               GUI_COLOR(GUI_COLOR_CHAT),
                               GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
        }
        
        pos++;
        if (pos[0])
        {
            while (pos && pos[0])
            {
                is_chanowner = 0;
                is_chanadmin = 0;
                is_chanadmin2 = 0;
                is_op = 0;
                is_halfop = 0;
                has_voice = 0;
                is_chanuser = 0;
                prefix_found = 1;
                
                while (prefix_found)
                {
                    prefix_found = 0;

                    if (irc_mode_nick_prefix_allowed (server, pos[0]))
                    {
                        prefix_found = 1;
                        switch (pos[0])
                        {
                            case '@': /* op */
                                is_op = 1;
                                color = GUI_COLOR_NICKLIST_PREFIX1;
                                break;
                            case '~': /* channel owner */
                                is_chanowner = 1;
                                color = GUI_COLOR_NICKLIST_PREFIX1;
                                break;
                            case '&': /* channel admin */
                                is_chanadmin = 1;
                                color = GUI_COLOR_NICKLIST_PREFIX1;
                                break;
                            case '!': /* channel admin (2) */
                                is_chanadmin2 = 1;
                                color = GUI_COLOR_NICKLIST_PREFIX1;
                                break;
                            case '%': /* half-op */
                                is_halfop = 1;
                                color = GUI_COLOR_NICKLIST_PREFIX2;
                                break;
                            case '+': /* voice */
                                has_voice = 1;
                                color = GUI_COLOR_NICKLIST_PREFIX3;
                                break;
                            case '-': /* channel user */
                                is_chanuser = 1;
                                color = GUI_COLOR_NICKLIST_PREFIX4;
                                break;
                            default:
                                color = GUI_COLOR_CHAT;
                                break;
                        }
                        if (!ignore && !ptr_channel)
                            gui_chat_printf (ptr_buffer, "%s%c",
                                        GUI_COLOR(color), pos[0]);
                    }
                    if (prefix_found)
                        pos++;
                }
                pos_nick = pos;
                pos = strchr (pos, ' ');
                if (pos)
                {
                    pos[0] = '\0';
                    pos++;
                }
                if (ptr_channel)
                {
                    if (!irc_nick_new (server, ptr_channel, pos_nick,
                                       is_chanowner, is_chanadmin, is_chanadmin2,
                                       is_op, is_halfop, has_voice, is_chanuser))
                    {
                        gui_chat_printf_error_nolog (server->buffer,
                                                _("%s cannot create nick \"%s\" "
                                                  "for channel \"%s\"\n"),
                                                WEECHAT_ERROR, pos_nick,
                                                ptr_channel->name);
                    }
                }
                else
                {
                    if (!ignore)
                    {
                        gui_chat_printf (ptr_buffer, "%s%s",
                                    GUI_COLOR(GUI_COLOR_CHAT), pos_nick);
                        if (pos && pos[0])
                            gui_chat_printf (ptr_buffer, " ");
                    }
                }
            }
        }
        if (ptr_channel)
        {
            gui_nicklist_draw (ptr_channel->buffer, 1, 1);
            gui_status_draw (ptr_channel->buffer, 1);
        }
        else
        {
            if (!ignore)
                gui_chat_printf (ptr_buffer, "%s]\n",
                            GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
        }
    }
    else
    {
        gui_chat_printf_error_nolog (server->buffer,
                                _("%s cannot parse \"%s\" command\n"),
                                WEECHAT_ERROR, "353");
        return -1;
    }
    return 0;
}

/*
 * irc_protocol_cmd_366: '366' command received (end of /names list)
 */

int
irc_protocol_cmd_366 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos, *pos2;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    int num_nicks, num_op, num_halfop, num_voice, num_normal;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
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
            
            ptr_channel = irc_channel_search (server, pos);
            if (ptr_channel)
            {
                if (!ignore)
                {
                    /* display users on channel */
                    gui_chat_printf_server (ptr_channel->buffer,
                                       _("Nicks %s%s%s: %s["),
                                       GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                       ptr_channel->name,
                                       GUI_COLOR(GUI_COLOR_CHAT),
                                       GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
                    
                    for (ptr_nick = ptr_channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
                    {
                        irc_display_nick (ptr_channel->buffer, ptr_nick, NULL,
                                          GUI_MSG_TYPE_MSG, 0,
                                          GUI_COLOR(GUI_COLOR_CHAT), 1);
                        if (ptr_nick != ptr_channel->last_nick)
                            gui_chat_printf (ptr_channel->buffer, " ");
                    }
                    gui_chat_printf (ptr_channel->buffer, "%s]\n",
                                GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
                    
                    /* display number of nicks, ops, halfops & voices on the channel */
                    irc_nick_count (ptr_channel, &num_nicks, &num_op, &num_halfop, &num_voice,
                                    &num_normal);
                    gui_chat_printf_info (ptr_channel->buffer,
                                     _("Channel %s%s%s: %s%d%s %s %s(%s%d%s %s, "
                                       "%s%d%s %s, %s%d%s %s, %s%d%s %s%s)\n"),
                                     GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                     ptr_channel->name,
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                     num_nicks,
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     (num_nicks > 1) ? _("nicks") : _("nick"),
                                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                     GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                     num_op,
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     (num_op > 1) ? _("ops") : _("op"),
                                     GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                     num_halfop,
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     (num_halfop > 1) ? _("halfops") : _("halfop"),
                                     GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                     num_voice,
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     (num_voice > 1) ? _("voices") : _("voice"),
                                     GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                     num_normal,
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     _("normal"),
                                     GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
                }
                irc_cmd_mode_server (server, ptr_channel->name);
                irc_channel_check_away (server, ptr_channel, 1);
            }
            else
            {
                if (!ignore)
                {
                    gui_chat_printf_info (gui_current_window->buffer,
                                     "%s%s%s: %s\n",
                                     GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                     pos,
                                     GUI_COLOR(GUI_COLOR_CHAT),
                                     pos2);
                }
                return 0;
            }
        }
    }
    return 0;
}

/*
 * irc_protocol_cmd_367: '367' command received (banlist)
 */

int
irc_protocol_cmd_367 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_channel, *pos_ban, *pos_user, *pos_date, *pos;
    t_irc_channel *ptr_channel;
    t_gui_buffer *buffer;
    time_t datetime;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    /* look for channel */
    pos_channel = strchr (arguments, ' ');
    if (!pos_channel)
    {
        gui_chat_printf_error_nolog (server->buffer,
                                _("%s cannot parse \"%s\" command\n"),
                                WEECHAT_ERROR, "367");
        return -1;
    }
    pos_channel[0] = '\0';
    pos_channel++;
    while (pos_channel[0] == ' ')
        pos_channel++;
    
    /* look for ban mask */
    pos_ban = strchr (pos_channel, ' ');
    if (!pos_ban)
    {
        gui_chat_printf_error_nolog (server->buffer,
                                _("%s cannot parse \"%s\" command\n"),
                                WEECHAT_ERROR, "367");
        return -1;
    }
    pos_ban[0] = '\0';
    pos_ban++;
    while (pos_ban[0] == ' ')
        pos_ban++;
    
    /* look for user who set ban */
    pos_date = NULL;
    pos_user = strchr (pos_ban, ' ');
    if (pos_user)
    {
        pos_user[0] = '\0';
        pos_user++;
        while (pos_user[0] == ' ')
            pos_user++;
        
        /* look for date/time */
        pos_date = strchr (pos_user, ' ');
        if (pos_date)
        {
            pos_date[0] = '\0';
            pos_date++;
            while (pos_date[0] == ' ')
                pos_date++;   
        }
    }
    
    ptr_channel = irc_channel_search (server, pos_channel);
    buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
    
    if (!ignore)
    {
        if (pos_user)
        {
            gui_chat_printf_info_nolog (buffer,
                                   _("%s[%s%s%s] %s%s%s banned by "),
                                   GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                   GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                   pos_channel,
                                   GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                   GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                   pos_ban,
                                   GUI_COLOR(GUI_COLOR_CHAT));
            pos = strchr (pos_user, '!');
            if (pos)
            {
                pos[0] = '\0';
                gui_chat_printf (buffer,
                            "%s%s %s(%s%s%s)",
                            GUI_COLOR(GUI_COLOR_CHAT_NICK),
                            pos_user,
                            GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                            GUI_COLOR(GUI_COLOR_CHAT_HOST),
                            pos + 1,
                            GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
            }
            else
                gui_chat_printf (buffer,
                            "%s%s",
                            GUI_COLOR(GUI_COLOR_CHAT_NICK),
                            pos_user);
            if (pos_date)
            {
                datetime = (time_t)(atol (pos_date));
                gui_chat_printf (buffer,
                            "%s, %s",
                            GUI_COLOR(GUI_COLOR_CHAT),
                            ctime (&datetime));
            }
            else
                gui_chat_printf (buffer, "\n");
        }
        else
            gui_chat_printf_info_nolog (buffer,
                                   _("%s[%s%s%s] %s%s%s banned\n"),
                                   GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                   GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                                   pos_channel,
                                   GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                   GUI_COLOR(GUI_COLOR_CHAT_HOST),
                                   pos_ban,
                                   GUI_COLOR(GUI_COLOR_CHAT));
    }    
    return 0;
}

/*
 * irc_protocol_cmd_368: '368' command received (end of banlist)
 */

int
irc_protocol_cmd_368 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_channel, *pos_msg;
    t_irc_channel *ptr_channel;
    t_gui_buffer *buffer;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    pos_channel = strchr (arguments, ' ');
    if (!pos_channel)
    {
        gui_chat_printf_error_nolog (server->buffer,
                                _("%s cannot parse \"%s\" command\n"),
                                WEECHAT_ERROR, "368");
        return -1;
    }
    pos_channel[0] = '\0';
    pos_channel++;
    while (pos_channel[0] == ' ')
        pos_channel++;
    
    pos_msg = strchr (pos_channel, ' ');
    if (!pos_msg)
    {
        gui_chat_printf_error_nolog (server->buffer,
                                _("%s cannot parse \"%s\" command\n"),
                                WEECHAT_ERROR, "368");
        return -1;
    }
    pos_msg[0] = '\0';
    pos_msg++;
    while (pos_msg[0] == ' ')
        pos_msg++;
    if (pos_msg[0] == ':')
        pos_msg++;
    
    ptr_channel = irc_channel_search (server, pos_channel);
    buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
    
    if (!ignore)
    {
        gui_chat_printf_info_nolog (buffer,
                               "%s[%s%s%s] %s%s\n",
                               GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                               GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                               pos_channel,
                               GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                               GUI_COLOR(GUI_COLOR_CHAT),
                               pos_msg);
    }    
    return 0;
}

/*
 * irc_protocol_cmd_432: '432' command received (erroneous nickname)
 */

int
irc_protocol_cmd_432 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    /* Note: this IRC command can not be ignored */
    
    irc_protocol_cmd_error (server, irc_message, host, nick, arguments,
                            ignore, highlight);
    
    if (!server->is_connected)
    {
        if (strcmp (server->nick, server->nick1) == 0)
        {
            gui_chat_printf_info (server->buffer,
                             _("%s: trying 2nd nickname \"%s\"\n"),
                             PACKAGE_NAME, server->nick2);
            free (server->nick);
            server->nick = strdup (server->nick2);
        }
        else
        {
            if (strcmp (server->nick, server->nick2) == 0)
            {
                gui_chat_printf_info (server->buffer,
                                 _("%s: trying 3rd nickname \"%s\"\n"),
                                 PACKAGE_NAME, server->nick3);
                free (server->nick);
                server->nick = strdup (server->nick3);
            }
            else
            {
                if (strcmp (server->nick, server->nick3) == 0)
                {
                    gui_chat_printf_info (server->buffer,
                                     _("%s: all declared nicknames are already in "
                                       "use or invalid, closing connection with "
                                       "server!\n"),
                                     PACKAGE_NAME);
                    irc_server_disconnect (server, 1);
                    return 0;
                }
                else
                {
                    gui_chat_printf_info (server->buffer,
                                     _("%s: trying 1st nickname \"%s\"\n"),
                                     PACKAGE_NAME, server->nick1);
                    free (server->nick);
                    server->nick = strdup (server->nick1);
                }
            }
        }
        irc_server_sendf (server, "NICK %s", server->nick);
    }
    return 0;
}

/*
 * irc_protocol_cmd_433: '433' command received (nickname already in use)
 */

int
irc_protocol_cmd_433 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    /* Note: this IRC command can not be ignored */
    
    if (!server->is_connected)
    {
        if (strcmp (server->nick, server->nick1) == 0)
        {
            gui_chat_printf_info (server->buffer,
                             _("%s: nickname \"%s\" is already in use, "
                               "trying 2nd nickname \"%s\"\n"),
                             PACKAGE_NAME, server->nick, server->nick2);
            free (server->nick);
            server->nick = strdup (server->nick2);
        }
        else
        {
            if (strcmp (server->nick, server->nick2) == 0)
            {
                gui_chat_printf_info (server->buffer,
                                 _("%s: nickname \"%s\" is already in use, "
                                   "trying 3rd nickname \"%s\"\n"),
                                 PACKAGE_NAME, server->nick, server->nick3);
                free (server->nick);
                server->nick = strdup (server->nick3);
            }
            else
            {
                if (strcmp (server->nick, server->nick3) == 0)
                {
                    gui_chat_printf_error (server->buffer,
                                      _("%s: all declared nicknames are already in use, "
                                        "closing connection with server!\n"),
                                      PACKAGE_NAME);
                    irc_server_disconnect (server, 1);
                    return 0;
                }
                else
                {
                    gui_chat_printf_info (server->buffer,
                                     _("%s: nickname \"%s\" is already in use, "
                                       "trying 1st nickname \"%s\"\n"),
                                     PACKAGE_NAME, server->nick, server->nick1);
                    free (server->nick);
                    server->nick = strdup (server->nick1);
                }
            }
        }
        irc_server_sendf (server, "NICK %s", server->nick);
    }
    else
        return irc_protocol_cmd_error (server, irc_message, host, nick, arguments,
                                       ignore, highlight);
    
    return 0;
}

/*
 * irc_protocol_cmd_438: '438' command received (not authorized to change nickname)
 */

int
irc_protocol_cmd_438 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos, *pos2;
    
    /* make C compiler happy */
    (void) server;
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
        pos = strchr (arguments, ' ');
        if (pos)
        {
            pos[0] = '\0';
            pos++;
            
            pos2 = strstr (pos, " :");
            if (pos2)
            {
                pos2[0] = '\0';
                pos2 += 2;
                gui_chat_printf_server (server->buffer,
                                   "%s (%s => %s)\n",
                                   pos2, arguments, pos);
            }
            else
                gui_chat_printf_server (server->buffer,
                                   "%s (%s)\n",
                                   pos, arguments);
        }
        else
            gui_chat_printf_server (server->buffer, "%s\n", arguments);
    }
    return 0;
}

/*
 * irc_protocol_cmd_671: '671' command (whois, secure connection)
 */

int
irc_protocol_cmd_671 (t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_nick, *pos_message;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
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
                
                gui_chat_printf_server (server->buffer,
                                   "%s[%s%s%s] %s%s\n",
                                   GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                   GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                   pos_nick,
                                   GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                   GUI_COLOR(GUI_COLOR_CHAT),
                                   pos_message);
            }
        }
    }
    return 0;
}
