/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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
 * irc_find_context: find window/buffer for a server/channel
 */

void
irc_find_context (t_irc_server *server, t_irc_channel *channel,
                  t_gui_window **window, t_gui_buffer **buffer)
{
    t_gui_window *ptr_win;
    
    if (!buffer)
        return;
    
    /* first find buffer */
    *buffer = NULL;
    if (channel && channel->buffer)
        *buffer = channel->buffer;
    else
    {
        if (server && server->buffer)
            *buffer = server->buffer;
        else
            *buffer = gui_current_window->buffer;
    }
    
    /* then find first window displaying this buffer */
    if (window)
    {
        *window = NULL;
        if (gui_current_window->buffer == *buffer)
            *window = gui_current_window;
        else
        {
            for (ptr_win = gui_windows; ptr_win;
                 ptr_win = ptr_win->next_window)
            {
                if (ptr_win->buffer == *buffer)
                {
                    *window = ptr_win;
                    break;
                }
            }
            if (!*window)
                *window = gui_current_window;
        }
    }
}

/*
 * irc_display_prefix: display a prefix for action/info/error msg
 *                     prefix must be 3 chars length
 */

void
irc_display_prefix (t_irc_server *server, t_gui_buffer *buffer, char *prefix)
{
    int type;
    
    type = MSG_TYPE_INFO | MSG_TYPE_PREFIX;
    
    if (!cfg_log_plugin_msg && (prefix == PREFIX_PLUGIN))
        type |= MSG_TYPE_NOLOG;
    
    if (prefix[0] == prefix[2])
    {
        gui_printf_type (buffer, type, "%s%c%s%c%s%c ",
                         GUI_COLOR(COLOR_WIN_CHAT_PREFIX1),
                         prefix[0],
                         GUI_COLOR(COLOR_WIN_CHAT_PREFIX2),
                         prefix[1],
                         GUI_COLOR(COLOR_WIN_CHAT_PREFIX1),
                         prefix[2]);
    }
    else
    {
        if (strcmp (prefix, PREFIX_JOIN) == 0)
            gui_printf_type (buffer, type, "%s%s ",
                             GUI_COLOR(COLOR_WIN_CHAT_JOIN), prefix);
        else if (strcmp (prefix, PREFIX_PART) == 0)
            gui_printf_type (buffer, type, "%s%s ",
                             GUI_COLOR(COLOR_WIN_CHAT_PART), prefix);
        else
            gui_printf_type (buffer, type, "%s%s ",
                             GUI_COLOR(COLOR_WIN_CHAT_PREFIX1), prefix);
    }
    if (server && (server->buffer == buffer) && buffer->all_servers)
    {
        gui_printf_type (buffer, type, "%s[%s%s%s] ",
                         GUI_COLOR(COLOR_WIN_CHAT_DARK),
                         GUI_COLOR(COLOR_WIN_CHAT_SERVER), server->name,
                         GUI_COLOR(COLOR_WIN_CHAT_DARK));
    }
    gui_printf_type (buffer, type, GUI_NO_COLOR);
}

/*
 * irc_display_nick: display nick in chat window
 *                   if color_nick < 0 then nick is highlighted
 */

void
irc_display_nick (t_gui_buffer *buffer, t_irc_nick *nick, char *nickname,
                  int type, int display_around, int color_nick, int no_nickmode)
{
    if (display_around)
        gui_printf_type (buffer, type, "%s%s",
                         GUI_COLOR(COLOR_WIN_CHAT_DARK),
                         (nick || BUFFER_IS_PRIVATE(buffer)) ? "<" : ">");
    if (nick && cfg_look_nickmode)
    {
        if (nick->flags & NICK_CHANOWNER)
            gui_printf_type (buffer, type, "%s~",
                             GUI_COLOR(COLOR_WIN_NICK_OP));
        else if (nick->flags & NICK_CHANADMIN)
            gui_printf_type (buffer, type, "%s&",
                             GUI_COLOR(COLOR_WIN_NICK_OP));
        else if (nick->flags & NICK_OP)
            gui_printf_type (buffer, type, "%s@",
                             GUI_COLOR(COLOR_WIN_NICK_OP));
        else if (nick->flags & NICK_HALFOP)
            gui_printf_type (buffer, type, "%s%%",
                             GUI_COLOR(COLOR_WIN_NICK_HALFOP));
        else if (nick->flags & NICK_VOICE)
            gui_printf_type (buffer, type, "%s+",
                             GUI_COLOR(COLOR_WIN_NICK_VOICE));
        else
            if (cfg_look_nickmode_empty && !no_nickmode)
                gui_printf_type (buffer, type, "%s ",
                                 GUI_COLOR(COLOR_WIN_CHAT));
    }
    if (color_nick < 0)
        gui_printf_type (buffer, type, "%s%s",
                         GUI_COLOR(COLOR_WIN_CHAT_HIGHLIGHT),
                         (nick) ? nick->nick : nickname);
    else
        gui_printf_type (buffer, type, "%s%s",
                         GUI_COLOR((nick && color_nick) ?
                                   nick->color : COLOR_WIN_CHAT),
                         (nick) ? nick->nick : nickname);
    
    if (display_around)
        gui_printf_type (buffer, type, "%s%s",
                         GUI_COLOR(COLOR_WIN_CHAT_DARK),
                         (nick || BUFFER_IS_PRIVATE(buffer)) ? "> " : "< ");
    gui_printf_type (buffer, type, GUI_NO_COLOR);
}

/*
 * irc_display_away: display away on all channels of all servers
 */

void
irc_display_away (t_irc_server *server, char *string1, char *string2)
{
    t_irc_channel *ptr_channel;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (ptr_channel->type == CHANNEL_TYPE_CHANNEL)
        {
            gui_printf_nolog (ptr_channel->buffer,
                              "%s[%s%s%s %s: %s%s]\n",
                              GUI_COLOR(COLOR_WIN_CHAT_DARK),
                              GUI_COLOR(COLOR_WIN_CHAT_NICK),
                              server->nick,
                              GUI_COLOR(COLOR_WIN_CHAT),
                              string1,
                              string2,
                              GUI_COLOR(COLOR_WIN_CHAT_DARK));
        }
    }
}

/*
 * irc_display_mode: display IRC message for mode change
 */

void
irc_display_mode (t_irc_server *server, t_gui_buffer *buffer,
                  char *channel_name, char set_flag,
                  char *symbol, char *nick_host, char *message, char *param)
{
    irc_display_prefix (server, buffer, PREFIX_INFO);
    gui_printf (buffer, "%s[%s%s%s/%s%c%s%s] %s%s",
                GUI_COLOR(COLOR_WIN_CHAT_DARK),
                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                channel_name,
                GUI_COLOR(COLOR_WIN_CHAT),
                GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                set_flag,
                symbol,
                GUI_COLOR(COLOR_WIN_CHAT_DARK),
                GUI_COLOR(COLOR_WIN_CHAT_NICK),
                nick_host);
    if (param)
        gui_printf (buffer, " %s%s %s%s\n",
                    GUI_COLOR(COLOR_WIN_CHAT),
                    message,
                    GUI_COLOR(COLOR_WIN_CHAT_NICK),
                    param);
    else
        gui_printf (buffer, " %s%s\n",
                    GUI_COLOR(COLOR_WIN_CHAT),
                    message);
}

/*
 * irc_display_server: display server description
 */

void
irc_display_server (t_irc_server *server)
{
    gui_printf (NULL, "\n");
    gui_printf (NULL, _("%sServer: %s%s %s[%s%s%s]\n"),
                GUI_COLOR(COLOR_WIN_CHAT),
                GUI_COLOR(COLOR_WIN_CHAT_SERVER),
                server->name,
                GUI_COLOR(COLOR_WIN_CHAT_DARK),
                GUI_COLOR(COLOR_WIN_CHAT),
                (server->is_connected) ?
                _("connected") : _("not connected"),
                GUI_COLOR(COLOR_WIN_CHAT_DARK));
    
    gui_printf (NULL, "  server_autoconnect . . . . : %s%s\n",
                (server->autoconnect) ? _("on") : _("off"),
                (server->command_line) ?
                _(" (temporary server, will not be saved)") : "");
    gui_printf (NULL, "  server_autoreconnect . . . : %s\n",
                (server->autoreconnect) ? _("on") : _("off"));
    gui_printf (NULL, "  server_autoreconnect_delay : %d %s\n",
                server->autoreconnect_delay,
                _("seconds"));
    gui_printf (NULL, "  server_address . . . . . . : %s\n",
                server->address);
    gui_printf (NULL, "  server_port  . . . . . . . : %d\n",
                server->port);
    gui_printf (NULL, "  server_ipv6  . . . . . . . : %s\n",
                (server->ipv6) ? _("on") : _("off"));
    gui_printf (NULL, "  server_ssl . . . . . . . . : %s\n",
                (server->ssl) ? _("on") : _("off"));
    gui_printf (NULL, "  server_password  . . . . . : %s\n",
                (server->password && server->password[0]) ?
                _("(hidden)") : "");
    gui_printf (NULL, "  server_nick1/2/3 . . . . . : %s %s/ %s%s %s/ %s%s\n",
                server->nick1,
                GUI_COLOR(COLOR_WIN_CHAT_DARK),
                GUI_COLOR(COLOR_WIN_CHAT),
                server->nick2,
                GUI_COLOR(COLOR_WIN_CHAT_DARK),
                GUI_COLOR(COLOR_WIN_CHAT),
                server->nick3);
    gui_printf (NULL, "  server_username  . . . . . : %s\n",
                server->username);
    gui_printf (NULL, "  server_realname  . . . . . : %s\n",
                server->realname);
    gui_printf (NULL, "  server_hostname  . . . . . : %s\n",
                (server->hostname) ? server->hostname : "");
    gui_printf (NULL, "  server_command . . . . . . : %s\n",
                (server->command && server->command[0]) ?
                server->command : "");
    gui_printf (NULL, "  server_command_delay . . . : %d %s\n",
                server->command_delay,
                _("seconds"));
    gui_printf (NULL, "  server_autojoin  . . . . . : %s\n",
                (server->autojoin && server->autojoin[0]) ?
                server->autojoin : "");
    gui_printf (NULL, "  server_notify_levels . . . : %s\n",
                (server->notify_levels && server->notify_levels[0]) ?
                server->notify_levels : "");
    gui_printf (NULL, "  server_charset_decode_iso. : %s\n",
                (server->charset_decode_iso && server->charset_decode_iso[0]) ?
                server->charset_decode_iso : "");
    gui_printf (NULL, "  server_charset_decode_utf. : %s\n",
                (server->charset_decode_utf && server->charset_decode_utf[0]) ?
                server->charset_decode_utf : "");
    gui_printf (NULL, "  server_charset_encode. . . : %s\n",
                (server->charset_encode && server->charset_encode[0]) ?
                server->charset_encode : "");
}
