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

/* gui-curses-status.c: status display functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../common/hotlist.h"
#include "../../common/weeconfig.h"
#include "gui-curses.h"


/*
 * gui_status_draw: draw status window for a buffer
 */

void
gui_status_draw (t_gui_buffer *buffer, int erase)
{
    t_gui_window *ptr_win;
    t_weechat_hotlist *ptr_hotlist;
    char format[32], str_nicks[32], *more;
    int i, first_mode, x, server_pos, server_total;
    int display_name, names_count;
    
    /* make gcc happy */
    (void) buffer;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (erase)
            gui_window_curses_clear (GUI_CURSES(ptr_win)->win_status, COLOR_WIN_STATUS);
        
        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status, COLOR_WIN_STATUS);
        
        /* display number of buffers */
        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                      COLOR_WIN_STATUS_DELIMITERS);
        mvwprintw (GUI_CURSES(ptr_win)->win_status, 0, 0, "[");
        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                      COLOR_WIN_STATUS);
        wprintw (GUI_CURSES(ptr_win)->win_status, "%d",
                 (last_gui_buffer) ? last_gui_buffer->number : 0);
        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                      COLOR_WIN_STATUS_DELIMITERS);
        wprintw (GUI_CURSES(ptr_win)->win_status, "] ");
        
        /* display "<servers>" or current server */
        if (ptr_win->buffer->all_servers)
        {
            wprintw (GUI_CURSES(ptr_win)->win_status, "[");
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                          COLOR_WIN_STATUS);
            wprintw (GUI_CURSES(ptr_win)->win_status, _("<servers>"));
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                          COLOR_WIN_STATUS_DELIMITERS);
            wprintw (GUI_CURSES(ptr_win)->win_status, "] ");
        }
        else if (SERVER(ptr_win->buffer) && SERVER(ptr_win->buffer)->name)
        {
            wprintw (GUI_CURSES(ptr_win)->win_status, "[");
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                          COLOR_WIN_STATUS);
            wprintw (GUI_CURSES(ptr_win)->win_status, "%s", SERVER(ptr_win->buffer)->name);
            if (SERVER(ptr_win->buffer)->is_away)
                wprintw (GUI_CURSES(ptr_win)->win_status, _("(away)"));
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                          COLOR_WIN_STATUS_DELIMITERS);
            wprintw (GUI_CURSES(ptr_win)->win_status, "] ");
        }
        
        /* infos about current server buffer */
        if (SERVER(ptr_win->buffer) && !CHANNEL(ptr_win->buffer))
        {
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                          COLOR_WIN_STATUS);
            wprintw (GUI_CURSES(ptr_win)->win_status, "%d",
                     ptr_win->buffer->number);
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                          COLOR_WIN_STATUS_DELIMITERS);
            wprintw (GUI_CURSES(ptr_win)->win_status, ":");
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                          COLOR_WIN_STATUS_CHANNEL);
            if (SERVER(ptr_win->buffer)->is_connected)
                wprintw (GUI_CURSES(ptr_win)->win_status, "[%s] ",
                         SERVER(ptr_win->buffer)->name);
            else
                wprintw (GUI_CURSES(ptr_win)->win_status, "(%s) ",
                         SERVER(ptr_win->buffer)->name);
            if (ptr_win->buffer->all_servers)
            {
                server_get_number_buffer (SERVER(ptr_win->buffer),
                                          &server_pos,
                                          &server_total);
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                              COLOR_WIN_STATUS_DELIMITERS);
                wprintw (GUI_CURSES(ptr_win)->win_status, "(");
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                              COLOR_WIN_STATUS);
                wprintw (GUI_CURSES(ptr_win)->win_status, "%d", server_pos);
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                              COLOR_WIN_STATUS_DELIMITERS);
                wprintw (GUI_CURSES(ptr_win)->win_status, "/");
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                              COLOR_WIN_STATUS);
                wprintw (GUI_CURSES(ptr_win)->win_status, "%d", server_total);
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                              COLOR_WIN_STATUS_DELIMITERS);
                wprintw (GUI_CURSES(ptr_win)->win_status, ") ");

            }
        }

        /* infos about current channel/pv buffer */
        if (SERVER(ptr_win->buffer) && CHANNEL(ptr_win->buffer))
        {
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                          COLOR_WIN_STATUS);
            wprintw (GUI_CURSES(ptr_win)->win_status, "%d",
                     ptr_win->buffer->number);
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                          COLOR_WIN_STATUS_DELIMITERS);
            wprintw (GUI_CURSES(ptr_win)->win_status, ":");
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                          COLOR_WIN_STATUS_CHANNEL);
            if (((CHANNEL(ptr_win->buffer)->type != CHANNEL_TYPE_PRIVATE)
                 && (CHANNEL(ptr_win->buffer)->type != CHANNEL_TYPE_DCC_CHAT)
                 && (!CHANNEL(ptr_win->buffer)->nicks))
                || ((CHANNEL(ptr_win->buffer)->type == CHANNEL_TYPE_DCC_CHAT)
                    && (CHANNEL(ptr_win->buffer)->dcc_chat)
                    && (((t_irc_dcc *)(CHANNEL(ptr_win->buffer)->dcc_chat))->sock < 0)))
                wprintw (GUI_CURSES(ptr_win)->win_status, "(%s)",
                         CHANNEL(ptr_win->buffer)->name);
            else
                wprintw (GUI_CURSES(ptr_win)->win_status, "%s",
                         CHANNEL(ptr_win->buffer)->name);
            if (ptr_win->buffer == CHANNEL(ptr_win->buffer)->buffer)
            {
                /* display channel modes */
                if (CHANNEL(ptr_win->buffer)->type == CHANNEL_TYPE_CHANNEL)
                {
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                  COLOR_WIN_STATUS_DELIMITERS);
                    wprintw (GUI_CURSES(ptr_win)->win_status, "(");
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                  COLOR_WIN_STATUS);
                    i = 0;
                    first_mode = 1;
                    while (CHANNEL(ptr_win->buffer)->modes[i])
                    {
                        if (CHANNEL(ptr_win->buffer)->modes[i] != ' ')
                        {
                            if (first_mode)
                            {
                                wprintw (GUI_CURSES(ptr_win)->win_status, "+");
                                first_mode = 0;
                            }
                            wprintw (GUI_CURSES(ptr_win)->win_status, "%c",
                                     CHANNEL(ptr_win->buffer)->modes[i]);
                        }
                        i++;
                    }
                    if (CHANNEL(ptr_win->buffer)->modes[CHANNEL_MODE_KEY] != ' ')
                        wprintw (GUI_CURSES(ptr_win)->win_status, ",%s",
                                 CHANNEL(ptr_win->buffer)->key);
                    if (CHANNEL(ptr_win->buffer)->modes[CHANNEL_MODE_LIMIT] != ' ')
                        wprintw (GUI_CURSES(ptr_win)->win_status, ",%d",
                                 CHANNEL(ptr_win->buffer)->limit);
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                  COLOR_WIN_STATUS_DELIMITERS);
                    wprintw (GUI_CURSES(ptr_win)->win_status, ")");
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                  COLOR_WIN_STATUS);
                }
                
                /* display DCC if private is DCC CHAT */
                if (CHANNEL(ptr_win->buffer)->type == CHANNEL_TYPE_DCC_CHAT)
                {
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                  COLOR_WIN_STATUS_DELIMITERS);
                    wprintw (GUI_CURSES(ptr_win)->win_status, "(");
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                  COLOR_WIN_STATUS_CHANNEL);
                    wprintw (GUI_CURSES(ptr_win)->win_status, "DCC");
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                  COLOR_WIN_STATUS_DELIMITERS);
                    wprintw (GUI_CURSES(ptr_win)->win_status, ")");
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                  COLOR_WIN_STATUS);
                }
            }
            wprintw (GUI_CURSES(ptr_win)->win_status, " ");
        }
        if (!SERVER(ptr_win->buffer))
        {
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status, COLOR_WIN_STATUS);
            wprintw (GUI_CURSES(ptr_win)->win_status, "%d",
                     ptr_win->buffer->number);
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status, COLOR_WIN_STATUS_DELIMITERS);
            wprintw (GUI_CURSES(ptr_win)->win_status, ":");
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status, COLOR_WIN_STATUS_CHANNEL);
            switch (ptr_win->buffer->type)
            {
                case BUFFER_TYPE_STANDARD:
                    wprintw (GUI_CURSES(ptr_win)->win_status, _("[not connected] "));
                    break;
                case BUFFER_TYPE_DCC:
                    wprintw (GUI_CURSES(ptr_win)->win_status, "<DCC> ");
                    break;
                case BUFFER_TYPE_RAW_DATA:
                    wprintw (GUI_CURSES(ptr_win)->win_status, _("<RAW_IRC> "));
                    break;
            }
        }
        
        /* display list of other active windows (if any) with numbers */
        if (hotlist)
        {
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                          COLOR_WIN_STATUS_DELIMITERS);
            wprintw (GUI_CURSES(ptr_win)->win_status, "[");
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status, COLOR_WIN_STATUS);
            wprintw (GUI_CURSES(ptr_win)->win_status, _("Act: "));
            
            names_count = 0;
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                switch (ptr_hotlist->priority)
                {
                    case HOTLIST_LOW:
                        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                      COLOR_WIN_STATUS_DATA_OTHER);
                        display_name = ((cfg_look_hotlist_names_level & 1) != 0);
                        break;
                    case HOTLIST_MSG:
                        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                      COLOR_WIN_STATUS_DATA_MSG);
                        display_name = ((cfg_look_hotlist_names_level & 2) != 0);
                        break;
                    case HOTLIST_PRIVATE:
                        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                      COLOR_WIN_STATUS_DATA_PRIVATE);
                        display_name = ((cfg_look_hotlist_names_level & 4) != 0);
                        break;
                    case HOTLIST_HIGHLIGHT:
                        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                      COLOR_WIN_STATUS_DATA_HIGHLIGHT);
                        display_name = ((cfg_look_hotlist_names_level & 8) != 0);
                        break;
                    default:
                        display_name = 0;
                        break;
                }
                switch (ptr_hotlist->buffer->type)
                {
                    case BUFFER_TYPE_STANDARD:
                        wprintw (GUI_CURSES(ptr_win)->win_status, "%d",
                                 ptr_hotlist->buffer->number);
                        
                        if (display_name && (cfg_look_hotlist_names_count != 0)
                            && (names_count < cfg_look_hotlist_names_count))
                        {
                            names_count++;
                            
                            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                          COLOR_WIN_STATUS_DELIMITERS);
                            wprintw (GUI_CURSES(ptr_win)->win_status, ":");
                            
                            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                          COLOR_WIN_STATUS);
                            if (cfg_look_hotlist_names_length == 0)
                                snprintf (format, sizeof (format) - 1, "%%s");
                            else
                                snprintf (format, sizeof (format) - 1, "%%.%ds", cfg_look_hotlist_names_length);
                            if (BUFFER_IS_SERVER(ptr_hotlist->buffer))
                                wprintw (GUI_CURSES(ptr_win)->win_status, format,
                                         (ptr_hotlist->server) ?
                                         ptr_hotlist->server->name :
                                         SERVER(ptr_hotlist->buffer)->name);
                            else if (BUFFER_IS_CHANNEL(ptr_hotlist->buffer)
                                     || BUFFER_IS_PRIVATE(ptr_hotlist->buffer))
                                wprintw (GUI_CURSES(ptr_win)->win_status, format, CHANNEL(ptr_hotlist->buffer)->name);
                        }
                        break;
                    case BUFFER_TYPE_DCC:
                        wprintw (GUI_CURSES(ptr_win)->win_status, "%d",
                                 ptr_hotlist->buffer->number);
                        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                      COLOR_WIN_STATUS_DELIMITERS);
                        wprintw (GUI_CURSES(ptr_win)->win_status, ":");
                        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                      COLOR_WIN_STATUS);
                        wprintw (GUI_CURSES(ptr_win)->win_status, "DCC");
                        break;
                    case BUFFER_TYPE_RAW_DATA:
                        wprintw (GUI_CURSES(ptr_win)->win_status, "%d",
                                 ptr_hotlist->buffer->number);
                        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                      COLOR_WIN_STATUS_DELIMITERS);
                        wprintw (GUI_CURSES(ptr_win)->win_status, ":");
                        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                      COLOR_WIN_STATUS);
                        wprintw (GUI_CURSES(ptr_win)->win_status, _("RAW_IRC"));
                        break;
                }
                
                if (ptr_hotlist->next_hotlist)
                    wprintw (GUI_CURSES(ptr_win)->win_status, ",");
            }
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                          COLOR_WIN_STATUS_DELIMITERS);
            wprintw (GUI_CURSES(ptr_win)->win_status, "] ");
        }
        
        /* display lag */
        if (SERVER(ptr_win->buffer))
        {
            if (SERVER(ptr_win->buffer)->lag / 1000 >= cfg_irc_lag_min_show)
            {
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                              COLOR_WIN_STATUS_DELIMITERS);
                wprintw (GUI_CURSES(ptr_win)->win_status, "[");
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status, COLOR_WIN_STATUS);
                wprintw (GUI_CURSES(ptr_win)->win_status, _("Lag: %.1f"),
                         ((float)(SERVER(ptr_win->buffer)->lag)) / 1000);
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                              COLOR_WIN_STATUS_DELIMITERS);
                wprintw (GUI_CURSES(ptr_win)->win_status, "]");
            }
        }
        
        /* display "-MORE-" (if last line is not displayed) & nicks count */
        if (BUFFER_HAS_NICKLIST(ptr_win->buffer))
        {
            snprintf (str_nicks, sizeof (str_nicks) - 1, "%d", CHANNEL(ptr_win->buffer)->nicks_count);
            x = ptr_win->win_status_width - strlen (str_nicks) - 4;
        }
        else
            x = ptr_win->win_status_width - 2;
        more = strdup (_("-MORE-"));
        x -= strlen (more) - 1;
        if (x < 0)
            x = 0;
        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status, COLOR_WIN_STATUS_MORE);
        if (ptr_win->scroll)
            mvwprintw (GUI_CURSES(ptr_win)->win_status, 0, x, "%s", more);
        else
        {
            snprintf (format, sizeof (format) - 1, "%%-%ds", (int)(strlen (more)));
            mvwprintw (GUI_CURSES(ptr_win)->win_status, 0, x, format, " ");
        }
        if (BUFFER_HAS_NICKLIST(ptr_win->buffer))
        {
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                          COLOR_WIN_STATUS_DELIMITERS);
            wprintw (GUI_CURSES(ptr_win)->win_status, " [");
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status, COLOR_WIN_STATUS);
            wprintw (GUI_CURSES(ptr_win)->win_status, "%s", str_nicks);
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                          COLOR_WIN_STATUS_DELIMITERS);
            wprintw (GUI_CURSES(ptr_win)->win_status, "]");
        }
        free (more);
        
        wnoutrefresh (GUI_CURSES(ptr_win)->win_status);
        refresh ();
    }
}
