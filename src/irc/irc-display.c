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
irc_display_prefix (t_gui_window *window, char *prefix)
{
    if (prefix[0] == prefix[2])
    {
        gui_printf_color (window, COLOR_WIN_CHAT_PREFIX1, "%c", prefix[0]);
        gui_printf_color (window, COLOR_WIN_CHAT_PREFIX2, "%c", prefix[1]);
        gui_printf_color (window, COLOR_WIN_CHAT_PREFIX1, "%c ", prefix[2]);
    }
    else
        gui_printf_color (window, COLOR_WIN_CHAT_PREFIX1, "%s ", prefix);
}

/*
 * irc_display_nick: display nick in chat window
 *                   if color_nick < 0 then nick is highlighted
 */

void
irc_display_nick (t_gui_window *window, t_irc_nick *nick, int message_type,
                  int display_around, int color_nick, int no_nickmode)
{
    if (display_around)
        gui_printf_color_type (window,
                               message_type, COLOR_WIN_CHAT_DARK, "<");
    if (cfg_look_nickmode)
    {
        if (nick->is_op)
            gui_printf_color_type (window,
                                   message_type,
                                   COLOR_WIN_NICK_OP, "@");
        else
        {
            if (nick->is_halfop)
                gui_printf_color_type (window,
                                       message_type,
                                       COLOR_WIN_NICK_HALFOP, "%%");
            else
            {
                if (nick->has_voice)
                    gui_printf_color_type (window,
                                           message_type,
                                           COLOR_WIN_NICK_VOICE, "+");
                else
                    if (cfg_look_nickmode_empty && !no_nickmode)
                        gui_printf_color_type (window,
                                               message_type,
                                               COLOR_WIN_CHAT, " ");
            }
        }
    }
    if (color_nick < 0)
        gui_printf_color_type (window,
                               message_type,
                               COLOR_WIN_CHAT_HIGHLIGHT,
                               "%s", nick->nick);
    else
        gui_printf_color_type (window,
                               message_type,
                               (color_nick) ?
                                   ((cfg_look_color_nicks) ?
                                   nick->color : COLOR_WIN_CHAT) :
                                   COLOR_WIN_CHAT,
                               "%s", nick->nick);
    
    if (display_around)
        gui_printf_color_type (window,
                               message_type, COLOR_WIN_CHAT_DARK, "> ");
}

/*
 * irc_display_mode: display IRC message for mode change
 */

void
irc_display_mode (t_gui_window *window, char *channel_name, char set_flag,
                  char *symbol, char *nick_host, char *message, char *param)
{
    irc_display_prefix (window, PREFIX_INFO);
    gui_printf_color (window, COLOR_WIN_CHAT_DARK, "[");
    gui_printf_color (window, COLOR_WIN_CHAT_CHANNEL, "%s", channel_name);
    gui_printf_color (window, COLOR_WIN_CHAT, "/");
    gui_printf_color (window, COLOR_WIN_CHAT_CHANNEL, "%c%s", set_flag, symbol);
    gui_printf_color (window, COLOR_WIN_CHAT_DARK, "] ");
    gui_printf_color (window, COLOR_WIN_CHAT_NICK, "%s", nick_host);
    if (param)
    {
        gui_printf_color (window, COLOR_WIN_CHAT, " %s ", message);
        gui_printf_color (window, COLOR_WIN_CHAT_NICK, "%s\n", param);
    }
    else
        gui_printf_color (window, COLOR_WIN_CHAT, " %s\n", message);
}
