/*
 * Copyright (c) 2003-2005 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
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

/* irc-display.c: display functions for IRC */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "../common/weechat.h"
#include "irc.h"
#include "../common/weeconfig.h"
#include "../gui/gui.h"


/*
 * irc_display_prefix: display prefix for action or info message
 *                     prefix must be 3 chars length
 */

void
irc_display_prefix (t_gui_buffer *buffer, char *prefix)
{
    int type;
    
    type = MSG_TYPE_INFO | MSG_TYPE_PREFIX;
    
    if (!cfg_log_plugin_msg && (prefix == PREFIX_PLUGIN))
        type |= MSG_TYPE_NOLOG;
    
    if (prefix[0] == prefix[2])
    {
        gui_printf_color_type (buffer, type, COLOR_WIN_CHAT_PREFIX1, "%c", prefix[0]);
        gui_printf_color_type (buffer, type, COLOR_WIN_CHAT_PREFIX2, "%c", prefix[1]);
        gui_printf_color_type (buffer, type, COLOR_WIN_CHAT_PREFIX1, "%c ", prefix[2]);
    }
    else
        gui_printf_color (buffer, COLOR_WIN_CHAT_PREFIX1, "%s ", prefix);
}

/*
 * irc_display_nick: display nick in chat window
 *                   if color_nick < 0 then nick is highlighted
 */

void
irc_display_nick (t_gui_buffer *buffer, t_irc_nick *nick, int message_type,
                  int display_around, int color_nick, int no_nickmode)
{
    if (display_around)
        gui_printf_color_type (buffer,
                               message_type, COLOR_WIN_CHAT_DARK, "<");
    if (cfg_look_nickmode)
    {
        if (nick->is_chanowner)
            gui_printf_color_type (buffer,
                                   message_type,
                                   COLOR_WIN_NICK_OP, "~");
        else if (nick->is_chanadmin)
            gui_printf_color_type (buffer,
                                   message_type,
                                   COLOR_WIN_NICK_OP, "&");
        else if (nick->is_op)
            gui_printf_color_type (buffer,
                                   message_type,
                                   COLOR_WIN_NICK_OP, "@");
        else if (nick->is_halfop)
            gui_printf_color_type (buffer,
                                   message_type,
                                   COLOR_WIN_NICK_HALFOP, "%%");
        else if (nick->has_voice)
            gui_printf_color_type (buffer,
                                   message_type,
                                   COLOR_WIN_NICK_VOICE, "+");
        else
            if (cfg_look_nickmode_empty && !no_nickmode)
                gui_printf_color_type (buffer,
                                       message_type,
                                       COLOR_WIN_CHAT, " ");
    }
    if (color_nick < 0)
        gui_printf_color_type (buffer,
                               message_type,
                               COLOR_WIN_CHAT_HIGHLIGHT,
                               "%s", nick->nick);
    else
        gui_printf_color_type (buffer,
                               message_type,
                               (color_nick) ?
                                   ((cfg_look_color_nicks) ?
                                   nick->color : COLOR_WIN_CHAT) :
                                   COLOR_WIN_CHAT,
                               "%s", nick->nick);
    
    if (display_around)
        gui_printf_color_type (buffer,
                               message_type, COLOR_WIN_CHAT_DARK, "> ");
}

/*
 * irc_display_mode: display IRC message for mode change
 */

void
irc_display_mode (t_gui_buffer *buffer, char *channel_name, char set_flag,
                  char *symbol, char *nick_host, char *message, char *param)
{
    irc_display_prefix (buffer, PREFIX_INFO);
    gui_printf_color (buffer, COLOR_WIN_CHAT_DARK, "[");
    gui_printf_color (buffer, COLOR_WIN_CHAT_CHANNEL, "%s", channel_name);
    gui_printf_color (buffer, COLOR_WIN_CHAT, "/");
    gui_printf_color (buffer, COLOR_WIN_CHAT_CHANNEL, "%c%s", set_flag, symbol);
    gui_printf_color (buffer, COLOR_WIN_CHAT_DARK, "] ");
    gui_printf_color (buffer, COLOR_WIN_CHAT_NICK, "%s", nick_host);
    if (param)
    {
        gui_printf_color (buffer, COLOR_WIN_CHAT, " %s ", message);
        gui_printf_color (buffer, COLOR_WIN_CHAT_NICK, "%s\n", param);
    }
    else
        gui_printf_color (buffer, COLOR_WIN_CHAT, " %s\n", message);
}

/*
 * irc_display_server: display server description
 */

void
irc_display_server (t_irc_server *server)
{
    gui_printf (NULL, "\n");
    gui_printf_color (NULL, COLOR_WIN_CHAT, _("Server: "));
    gui_printf_color (NULL, COLOR_WIN_CHAT_CHANNEL, "%s", server->name);
    gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, " [");
    gui_printf_color (NULL, COLOR_WIN_CHAT, "%s",
                      (server->is_connected) ?
                          _("connected") : _("not connected"));
    gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, "]\n");
    gui_printf_color (NULL, COLOR_WIN_CHAT,
                      "  server_autoconnect        : %s%s\n",
                      (server->autoconnect) ? _("yes") : _("no"),
                      (server->command_line) ?
                          _(" (temporary server, will not be saved)") : "");
    gui_printf_color (NULL, COLOR_WIN_CHAT,
                      "  server_autoreconnect . . .: %s\n",
                      (server->autoreconnect) ? _("yes") : _("no"));
    gui_printf_color (NULL, COLOR_WIN_CHAT,
                      "  server_autoreconnect_delay: %d seconds\n",
                      server->autoreconnect_delay);
    gui_printf_color (NULL, COLOR_WIN_CHAT,
                      "  server_address . . . . . .: %s\n",
                      server->address);
    gui_printf_color (NULL, COLOR_WIN_CHAT,
                      "  server_port  . . . . . . .: %d\n",
                      server->port);
    gui_printf_color (NULL, COLOR_WIN_CHAT,
                      "  server_password  . . . . .: %s\n",
                      (server->password && server->password[0]) ?
                      _("(hidden)") : "");
    gui_printf_color (NULL, COLOR_WIN_CHAT,
                      "  server_nick1/2/3 . . . . .: %s", server->nick1);
    gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, " / ");
    gui_printf_color (NULL, COLOR_WIN_CHAT, "%s", server->nick2);
    gui_printf_color (NULL, COLOR_WIN_CHAT_DARK, " / ");
    gui_printf_color (NULL, COLOR_WIN_CHAT, "%s\n", server->nick3);
    gui_printf_color (NULL, COLOR_WIN_CHAT,
                      "  server_username  . . . . .: %s\n",
                      server->username);
    gui_printf_color (NULL, COLOR_WIN_CHAT,
                      "  server_realname  . . . . .: %s\n",
                      server->realname);
    gui_printf_color (NULL, COLOR_WIN_CHAT,
                      "  server_command . . . . . .: %s\n",
                      (server->command && server->command[0]) ?
                      server->command : "");
    gui_printf_color (NULL, COLOR_WIN_CHAT,
                      "  server_command_delay . . .: %d seconds\n",
                      server->command_delay);
    gui_printf_color (NULL, COLOR_WIN_CHAT,
                      "  server_autojoin  . . . . .: %s\n",
                      (server->autojoin && server->autojoin[0]) ?
                      server->autojoin : "");
}
