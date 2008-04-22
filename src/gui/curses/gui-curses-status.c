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

/* gui-curses-status.c: status display functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-utf8.h"
#include "../../plugins/plugin.h"
#include "../gui-status.h"
#include "../gui-buffer.h"
#include "../gui-color.h"
#include "../gui-main.h"
#include "../gui-hotlist.h"
#include "../gui-window.h"
#include "gui-curses.h"


/*
 * gui_status_draw: draw status window
 */

void
gui_status_draw (int erase)
{
    struct t_gui_window *ptr_win;
    struct t_gui_hotlist *ptr_hotlist;
    char format[32], *more;
    int x;
    int display_name, names_count;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (erase)
            gui_window_curses_clear (GUI_CURSES(ptr_win)->win_status,
                                     GUI_COLOR_STATUS);
        
        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                      GUI_COLOR_STATUS);
        
        /* display number of buffers */
        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                      GUI_COLOR_STATUS_DELIMITERS);
        mvwprintw (GUI_CURSES(ptr_win)->win_status, 0, 0, "[");
        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                      GUI_COLOR_STATUS);
        wprintw (GUI_CURSES(ptr_win)->win_status, "%d",
                 (last_gui_buffer) ? last_gui_buffer->number : 0);
        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                      GUI_COLOR_STATUS_DELIMITERS);
        wprintw (GUI_CURSES(ptr_win)->win_status, "] ");
        
        /* display buffer plugin */
        wprintw (GUI_CURSES(ptr_win)->win_status, "[");
        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                      GUI_COLOR_STATUS);
        wprintw (GUI_CURSES(ptr_win)->win_status, "%s",
                 (ptr_win->buffer->plugin) ?
                 ptr_win->buffer->plugin->name : "core");
        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                      GUI_COLOR_STATUS_DELIMITERS);
        wprintw (GUI_CURSES(ptr_win)->win_status, "] ");
        
        /* display buffer number/category/name */
        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                      GUI_COLOR_STATUS_NUMBER);
        wprintw (GUI_CURSES(ptr_win)->win_status, "%d",
                 ptr_win->buffer->number);
        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                      GUI_COLOR_STATUS_DELIMITERS);
        wprintw (GUI_CURSES(ptr_win)->win_status, ":");
        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                      GUI_COLOR_STATUS_CATEGORY);
        wprintw (GUI_CURSES(ptr_win)->win_status, "%s",
                 ptr_win->buffer->category);
        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                      GUI_COLOR_STATUS_DELIMITERS);
        wprintw (GUI_CURSES(ptr_win)->win_status, "/");
        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                      GUI_COLOR_STATUS_NAME);
        gui_window_wprintw (GUI_CURSES(ptr_win)->win_status, "%s ",
                            ptr_win->buffer->name);
        
        /* display list of other active windows (if any) with numbers */
        if (gui_hotlist)
        {
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                          GUI_COLOR_STATUS_DELIMITERS);
            wprintw (GUI_CURSES(ptr_win)->win_status, "[");
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                          GUI_COLOR_STATUS);
            gui_window_wprintw (GUI_CURSES(ptr_win)->win_status, _("Act: "));
            
            names_count = 0;
            for (ptr_hotlist = gui_hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                switch (ptr_hotlist->priority)
                {
                    case GUI_HOTLIST_LOW:
                        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                      GUI_COLOR_STATUS_DATA_OTHER);
                        display_name = ((CONFIG_INTEGER(config_look_hotlist_names_level) & 1) != 0);
                        break;
                    case GUI_HOTLIST_MESSAGE:
                        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                      GUI_COLOR_STATUS_DATA_MSG);
                        display_name = ((CONFIG_INTEGER(config_look_hotlist_names_level) & 2) != 0);
                        break;
                    case GUI_HOTLIST_PRIVATE:
                        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                      GUI_COLOR_STATUS_DATA_PRIVATE);
                        display_name = ((CONFIG_INTEGER(config_look_hotlist_names_level) & 4) != 0);
                        break;
                    case GUI_HOTLIST_HIGHLIGHT:
                        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                      GUI_COLOR_STATUS_DATA_HIGHLIGHT);
                        display_name = ((CONFIG_INTEGER(config_look_hotlist_names_level) & 8) != 0);
                        break;
                    default:
                        display_name = 0;
                        break;
                }

                wprintw (GUI_CURSES(ptr_win)->win_status, "%d",
                         ptr_hotlist->buffer->number);
                
                if (display_name
                    && (CONFIG_INTEGER(config_look_hotlist_names_count) != 0)
                    && (names_count < CONFIG_INTEGER(config_look_hotlist_names_count)))
                {
                    names_count++;
                    
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                  GUI_COLOR_STATUS_DELIMITERS);
                    wprintw (GUI_CURSES(ptr_win)->win_status, ":");
                    
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                                  GUI_COLOR_STATUS);
                    if (CONFIG_INTEGER(config_look_hotlist_names_length) == 0)
                        snprintf (format, sizeof (format) - 1, "%%s");
                    else
                        snprintf (format, sizeof (format) - 1,
                                  "%%.%ds",
                                  CONFIG_INTEGER(config_look_hotlist_names_length));
                    gui_window_wprintw (GUI_CURSES(ptr_win)->win_status,
                                        format,
                                        ptr_hotlist->buffer->name);
                }
                
                if (ptr_hotlist->next_hotlist)
                    wprintw (GUI_CURSES(ptr_win)->win_status, ",");
            }
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                          GUI_COLOR_STATUS_DELIMITERS);
            wprintw (GUI_CURSES(ptr_win)->win_status, "] ");
        }
        
        /* display lag */
        /*if (GUI_SERVER(ptr_win->buffer))
        {
            if (GUI_SERVER(ptr_win->buffer)->lag / 1000 >= config_irc_lag_min_show)
            {
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                              GUI_COLOR_STATUS_DELIMITERS);
                wprintw (GUI_CURSES(ptr_win)->win_status, "[");
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                              GUI_COLOR_STATUS);
                gui_window_wprintw (GUI_CURSES(ptr_win)->win_status,
                                    _("Lag: %.1f"),
                                    ((float)(GUI_SERVER(ptr_win->buffer)->lag)) / 1000);
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                              GUI_COLOR_STATUS_DELIMITERS);
                wprintw (GUI_CURSES(ptr_win)->win_status, "]");
            }
        }*/
        
        /* display "-MORE-" (if last line is not displayed) & nicks count */
        /*if (ptr_win->buffer->attribs & GUI_BUFFER_ATTRIB_NICKLIST)
        {
            snprintf (str_nicks, sizeof (str_nicks) - 1, "%d",
                      GUI_CHANNEL(ptr_win->buffer)->nicks_count);
            x = ptr_win->win_status_width - utf8_strlen (str_nicks) - 4;
        }
        else*/
            x = ptr_win->win_status_width - 1;
        more = strdup (_("-MORE-"));
        if (more)
        {
            x -= utf8_strlen (more) - 1;
            if (x < 0)
                x = 0;
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                          GUI_COLOR_STATUS_MORE);
            if (ptr_win->scroll)
            {
                wmove (GUI_CURSES(ptr_win)->win_status, 0, x);
                gui_window_wprintw (GUI_CURSES(ptr_win)->win_status, "%s", more);
            }
            else
            {
                /*snprintf (format, sizeof (format) - 1,
                  "%%-%ds", (int)(utf8_strlen (more)));
                  wmove (GUI_CURSES(ptr_win)->win_status, 0, x);
                  gui_window_wprintw (GUI_CURSES(ptr_win)->win_status, format, " ");
                */
            }
            /*if (ptr_win->buffer->attribs & GUI_BUFFER_ATTRIB_NICKLIST)
            {
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                              GUI_COLOR_STATUS_DELIMITERS);
                wprintw (GUI_CURSES(ptr_win)->win_status, " [");
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                              GUI_COLOR_STATUS);
                wprintw (GUI_CURSES(ptr_win)->win_status, "%s", str_nicks);
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_status,
                                              GUI_COLOR_STATUS_DELIMITERS);
                wprintw (GUI_CURSES(ptr_win)->win_status, "]");
            }*/
            free (more);
        }
        
        wnoutrefresh (GUI_CURSES(ptr_win)->win_status);
        refresh ();
    }
    
    gui_status_refresh_needed = 0;
}
