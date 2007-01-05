/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

/* gui-curses-nicklist.c: nicklist display functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../common/weeconfig.h"
#include "../../irc/irc.h"
#include "gui-curses.h"


/*
 * gui_nicklist_draw: draw nick window for a buffer
 */

void
gui_nicklist_draw (t_gui_buffer *buffer, int erase, int calculate_size)
{
    t_gui_window *ptr_win;
    int i, j, x, y, x2, column, max_length, nicks_displayed;
    char format[32], format_empty[32];
    t_irc_nick *ptr_nick;
    
    if (!gui_ok || !BUFFER_HAS_NICKLIST(buffer))
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if ((ptr_win->buffer == buffer) && (buffer->num_displayed > 0))
        {
            max_length = nick_get_max_length (CHANNEL(buffer));
            
            if (calculate_size && (gui_window_calculate_pos_size (ptr_win, 0)))
            {
                delwin (GUI_CURSES(ptr_win)->win_chat);
                delwin (GUI_CURSES(ptr_win)->win_nick);
                GUI_CURSES(ptr_win)->win_chat = newwin (ptr_win->win_chat_height,
                                                        ptr_win->win_chat_width,
                                                        ptr_win->win_chat_y,
                                                        ptr_win->win_chat_x);
                GUI_CURSES(ptr_win)->win_nick = newwin (ptr_win->win_nick_height,
                                                        ptr_win->win_nick_width,
                                                        ptr_win->win_nick_y,
                                                        ptr_win->win_nick_x);
                gui_chat_draw (buffer, 1);
                erase = 1;
            }
            
            if (erase)
            {
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_nick, COLOR_WIN_NICK);
                
                snprintf (format_empty, 32, "%%-%ds", ptr_win->win_nick_width);
                for (i = 0; i < ptr_win->win_nick_height; i++)
                {
                    mvwprintw (GUI_CURSES(ptr_win)->win_nick, i, 0, format_empty, " ");
                }
            }
            
            if ((cfg_look_nicklist_position == CFG_LOOK_NICKLIST_TOP) ||
                (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_BOTTOM))
                snprintf (format, 32, "%%.%ds", max_length);
            else
                snprintf (format, 32, "%%.%ds",
                          ((cfg_look_nicklist_min_size > 0)
                           && (max_length < cfg_look_nicklist_min_size)) ?
                          cfg_look_nicklist_min_size :
                          (((cfg_look_nicklist_max_size > 0)
                            && (max_length > cfg_look_nicklist_max_size)) ?
                           cfg_look_nicklist_max_size : max_length));
            
            if (cfg_look_nicklist_separator && has_colors ())
            {
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_nick, COLOR_WIN_NICK_SEP);
                switch (cfg_look_nicklist_position)
                {
                    case CFG_LOOK_NICKLIST_LEFT:
                        mvwvline (GUI_CURSES(ptr_win)->win_nick,
                                  0, ptr_win->win_nick_width - 1, ACS_VLINE,
                                  ptr_win->win_chat_height);
                        break;
                    case CFG_LOOK_NICKLIST_RIGHT:
                        mvwvline (GUI_CURSES(ptr_win)->win_nick,
                                  0, 0, ACS_VLINE,
                                  ptr_win->win_chat_height);
                        break;
                    case CFG_LOOK_NICKLIST_TOP:
                        mvwhline (GUI_CURSES(ptr_win)->win_nick,
                                  ptr_win->win_nick_height - 1, 0, ACS_HLINE,
                                  ptr_win->win_chat_width);
                        break;
                    case CFG_LOOK_NICKLIST_BOTTOM:
                        mvwhline (GUI_CURSES(ptr_win)->win_nick,
                                  0, 0, ACS_HLINE,
                                  ptr_win->win_chat_width);
                        break;
                }
            }
            
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_nick, COLOR_WIN_NICK);
            x = 0;
            y = (cfg_look_nicklist_separator && (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_BOTTOM)) ? 1 : 0;
            column = 0;
            
            if ((cfg_look_nicklist_position == CFG_LOOK_NICKLIST_TOP) ||
                (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_BOTTOM))
                nicks_displayed = (ptr_win->win_width / (max_length + 2)) * (ptr_win->win_nick_height - cfg_look_nicklist_separator);
            else
                nicks_displayed = ptr_win->win_nick_height;
            
            ptr_nick = CHANNEL(buffer)->nicks;
            for (i = 0; i < ptr_win->win_nick_start; i++)
            {
                if (!ptr_nick)
                    break;
                ptr_nick = ptr_nick->next_nick;
            }
            if (ptr_nick)
            {
                for (i = 0; i < nicks_displayed; i++)
                {
                    switch (cfg_look_nicklist_position)
                    {
                        case CFG_LOOK_NICKLIST_LEFT:
                            x = 0;
                            break;
                        case CFG_LOOK_NICKLIST_RIGHT:
                            x = cfg_look_nicklist_separator;
                            break;
                        case CFG_LOOK_NICKLIST_TOP:
                        case CFG_LOOK_NICKLIST_BOTTOM:
                            x = column;
                            break;
                    }
                    if ( ((i == 0) && (ptr_win->win_nick_start > 0))
                         || ((i == nicks_displayed - 1) && (ptr_nick->next_nick)) )
                    {
                        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_nick, COLOR_WIN_NICK_MORE);
                        j = (max_length + 1) >= 4 ? 4 : max_length + 1;
                        for (x2 = 1; x2 <= j; x2++)
                            mvwprintw (GUI_CURSES(ptr_win)->win_nick, y, x + x2, "+");
                    }
                    else
                    {
                        if (ptr_nick->flags & NICK_CHANOWNER)
                        {
                            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_nick, COLOR_WIN_NICK_CHANOWNER);
                            mvwprintw (GUI_CURSES(ptr_win)->win_nick, y, x, "~");
                            x++;
                        }
                        else if (ptr_nick->flags & NICK_CHANADMIN)
                        {
                            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_nick, COLOR_WIN_NICK_CHANADMIN);
                            mvwprintw (GUI_CURSES(ptr_win)->win_nick, y, x, "&");
                            x++;
                        }
                        else if (ptr_nick->flags & NICK_OP)
                        {
                            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_nick, COLOR_WIN_NICK_OP);
                            mvwprintw (GUI_CURSES(ptr_win)->win_nick, y, x, "@");
                            x++;
                        }
                        else if (ptr_nick->flags & NICK_HALFOP)
                        {
                            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_nick, COLOR_WIN_NICK_HALFOP);
                            mvwprintw (GUI_CURSES(ptr_win)->win_nick, y, x, "%%");
                            x++;
                        }
                        else if (ptr_nick->flags & NICK_VOICE)
                        {
                            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_nick, COLOR_WIN_NICK_VOICE);
                            mvwprintw (GUI_CURSES(ptr_win)->win_nick, y, x, "+");
                            x++;
                        }
                        else
                        {
                            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_nick, COLOR_WIN_NICK);
                            mvwprintw (GUI_CURSES(ptr_win)->win_nick, y, x, " ");
                            x++;
                        }
                        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_nick,
                                                      ((cfg_irc_away_check > 0) && (ptr_nick->flags & NICK_AWAY)) ?
                                                      COLOR_WIN_NICK_AWAY : COLOR_WIN_NICK);
                        mvwprintw (GUI_CURSES(ptr_win)->win_nick, y, x, format, ptr_nick->nick);
                        
                        ptr_nick = ptr_nick->next_nick;
                        
                        if (!ptr_nick)
                            break;
                    }
                    y++;
                    if ((cfg_look_nicklist_position == CFG_LOOK_NICKLIST_TOP) ||
                        (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_BOTTOM))
                    {
                        if (y - ((cfg_look_nicklist_separator
                                  && (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_BOTTOM)) ? 1 : 0)
                            >= ptr_win->win_nick_height - 1)
                        {
                            column += max_length + 2;
                            y = (cfg_look_nicklist_separator && (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_BOTTOM)) ?
                                1 : 0;
                        }
                    }
                }
            }
            wnoutrefresh (GUI_CURSES(ptr_win)->win_nick);
            refresh ();
        }
    }
}
