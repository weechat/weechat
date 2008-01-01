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

/* gui-curses-nicklist.c: nicklist display functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-string.h"
#include "../../core/wee-utf8.h"
#include "../gui-nicklist.h"
#include "../gui-buffer.h"
#include "../gui-chat.h"
#include "../gui-color.h"
#include "../gui-main.h"
#include "../gui-window.h"
#include "gui-curses.h"


/*
 * gui_nicklist_draw: draw nick window for a buffer
 */

void
gui_nicklist_draw (struct t_gui_buffer *buffer, int erase)
{
    struct t_gui_window *ptr_win;
    struct t_gui_nick_group *ptr_group;
    struct t_gui_nick *ptr_nick;
    int i, j, k, x, y, x2, max_y, column, max_length, max_chars;
    int nicks_displayed, chars_left;
    char format_empty[32], *buf, *ptr_buf, *ptr_next, saved_char;
    
    if (!gui_ok || (!buffer->nicklist))
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if ((ptr_win->buffer == buffer) && (buffer->num_displayed > 0))
        {
            max_length = gui_nicklist_get_max_length (buffer, NULL);
            if ((max_length != buffer->nicklist_max_length)
                || (buffer->nicklist && !GUI_CURSES(ptr_win)->win_nick)
                || (!buffer->nicklist && GUI_CURSES(ptr_win)->win_nick))
            {
                buffer->nicklist_max_length = max_length;
                if (gui_window_calculate_pos_size (ptr_win, 0))
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
            }
            
            if (erase)
            {
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_nick,
                                              GUI_COLOR_NICKLIST);
                
                snprintf (format_empty, 32, "%%-%ds", ptr_win->win_nick_width);
                for (i = 0; i < ptr_win->win_nick_height; i++)
                {
                    mvwprintw (GUI_CURSES(ptr_win)->win_nick, i, 0,
                               format_empty, " ");
                }
            }
            
            if ((CONFIG_INTEGER(config_look_nicklist_position) == CONFIG_LOOK_NICKLIST_TOP) ||
                (CONFIG_INTEGER(config_look_nicklist_position) == CONFIG_LOOK_NICKLIST_BOTTOM))
                max_chars = max_length;
            else
                max_chars = ((CONFIG_INTEGER(config_look_nicklist_min_size) > 0)
                             && (max_length < CONFIG_INTEGER(config_look_nicklist_min_size))) ?
                    CONFIG_INTEGER(config_look_nicklist_min_size) :
                    (((CONFIG_INTEGER(config_look_nicklist_max_size) > 0)
                      && (max_length > CONFIG_INTEGER(config_look_nicklist_max_size))) ?
                     CONFIG_INTEGER(config_look_nicklist_max_size) : max_length);
            
            if (CONFIG_BOOLEAN(config_look_nicklist_separator) && has_colors ())
            {
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_nick,
                                              GUI_COLOR_NICKLIST_SEPARATOR);
                switch (CONFIG_INTEGER(config_look_nicklist_position))
                {
                    case CONFIG_LOOK_NICKLIST_LEFT:
                        mvwvline (GUI_CURSES(ptr_win)->win_nick,
                                  0, ptr_win->win_nick_width - 1, ACS_VLINE,
                                  ptr_win->win_chat_height);
                        break;
                    case CONFIG_LOOK_NICKLIST_RIGHT:
                        mvwvline (GUI_CURSES(ptr_win)->win_nick,
                                  0, 0, ACS_VLINE,
                                  ptr_win->win_chat_height);
                        break;
                    case CONFIG_LOOK_NICKLIST_TOP:
                        mvwhline (GUI_CURSES(ptr_win)->win_nick,
                                  ptr_win->win_nick_height - 1, 0, ACS_HLINE,
                                  ptr_win->win_chat_width);
                        break;
                    case CONFIG_LOOK_NICKLIST_BOTTOM:
                        mvwhline (GUI_CURSES(ptr_win)->win_nick,
                                  0, 0, ACS_HLINE,
                                  ptr_win->win_chat_width);
                        break;
                }
            }
            
            x = 0;
            y = (CONFIG_BOOLEAN(config_look_nicklist_separator)
                 && (CONFIG_INTEGER(config_look_nicklist_position) == CONFIG_LOOK_NICKLIST_BOTTOM)) ?
                1 : 0;
            max_y = 0;
            switch (CONFIG_INTEGER(config_look_nicklist_position))
            {
                case CONFIG_LOOK_NICKLIST_LEFT:
                case CONFIG_LOOK_NICKLIST_RIGHT:
                    max_y = 0;
                    break;
                case CONFIG_LOOK_NICKLIST_TOP:
                    max_y = ptr_win->win_nick_height -
                        ((CONFIG_BOOLEAN(config_look_nicklist_separator)) ? 1 : 0);
                    break;
                case CONFIG_LOOK_NICKLIST_BOTTOM:
                    max_y = ptr_win->win_nick_height;
                    break;
            }
            column = 0;
            
            if ((CONFIG_INTEGER(config_look_nicklist_position) == CONFIG_LOOK_NICKLIST_TOP) ||
                (CONFIG_INTEGER(config_look_nicklist_position) == CONFIG_LOOK_NICKLIST_BOTTOM))
                nicks_displayed = (ptr_win->win_width / (max_length + 2)) *
                    (ptr_win->win_nick_height -
                     ((CONFIG_BOOLEAN(config_look_nicklist_separator)) ? 1 : 0));
            else
                nicks_displayed = ptr_win->win_nick_height;
            
            ptr_group = NULL;
            ptr_nick = NULL;
            gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
            i = 0;
            while ((ptr_group || ptr_nick) && (i < ptr_win->win_nick_start))
            {
                if ((ptr_nick && ptr_nick->visible)
                    || (ptr_group && buffer->nicklist_display_groups
                        && ptr_group->visible))
                    i++;
                gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
            }
            i = 0;
            while ((ptr_group || ptr_nick) && (i < nicks_displayed))
            {
                if ((ptr_nick && ptr_nick->visible)
                    || (ptr_group && buffer->nicklist_display_groups
                        && ptr_group->visible))
                {
                    i++;
                    switch (CONFIG_INTEGER(config_look_nicklist_position))
                    {
                        case CONFIG_LOOK_NICKLIST_LEFT:
                            x = 0;
                            break;
                        case CONFIG_LOOK_NICKLIST_RIGHT:
                            x = (CONFIG_BOOLEAN(config_look_nicklist_separator)) ? 1 : 0;
                            break;
                        case CONFIG_LOOK_NICKLIST_TOP:
                        case CONFIG_LOOK_NICKLIST_BOTTOM:
                            x = column;
                            break;
                    }
                    if ( ((i == 0) && (ptr_win->win_nick_start > 0))
                         || ((i == nicks_displayed - 1) && (ptr_nick->next_nick)) )
                    {
                        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_nick,
                                                      GUI_COLOR_NICKLIST_MORE);
                        j = (max_length + 1) >= 4 ? 4 : max_length + 1;
                        for (x2 = 1; x2 <= j; x2++)
                            mvwprintw (GUI_CURSES(ptr_win)->win_nick,
                                       y, x + x2, "+");
                    }
                    else
                    {
                        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_nick,
                                                      GUI_COLOR_NICKLIST);
                        if (ptr_nick)
                        {
                            /* display spaces and prefix for nick */
                            if (buffer->nicklist_display_groups)
                            {
                                for (k = 0; k < ptr_nick->group->level; k++)
                                {
                                    mvwprintw (GUI_CURSES(ptr_win)->win_nick,
                                               y, x, " ");
                                    x++;
                                }
                            }
                            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_nick,
                                                          ptr_nick->prefix_color);
                            mvwprintw (GUI_CURSES(ptr_win)->win_nick,
                                       y, x, "%c", ptr_nick->prefix);
                            x++;
                            
                            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_nick,
                                                          ptr_nick->color);
                            ptr_buf = ptr_nick->name;
                        }
                        else
                        {
                            /* display group name */
                            for (k = 0; k < ptr_group->level - 1; k++)
                            {
                                mvwprintw (GUI_CURSES(ptr_win)->win_nick,
                                           y, x, " ");
                                x++;
                            }
                            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_nick,
                                                          ptr_group->color);
                            //wattron (GUI_CURSES(ptr_win)->win_nick, A_UNDERLINE);
                            ptr_buf = gui_nicklist_get_group_start (ptr_group->name);
                        }
                        wmove (GUI_CURSES(ptr_win)->win_nick, y, x);
                        saved_char = '\0';
                        chars_left = max_chars - ((ptr_nick) ? 1 : 0);
                        for (k = 0; k < chars_left; k++)
                        {
                            if (ptr_buf && ptr_buf[0])
                            {
                                ptr_next = utf8_next_char (ptr_buf);
                                if (ptr_next)
                                {
                                    saved_char = ptr_next[0];
                                    ptr_next[0] = '\0';
                                }
                                buf = string_iconv_from_internal (NULL,
                                                                  ptr_buf);
                                wprintw (GUI_CURSES(ptr_win)->win_nick, "%s",
                                         (buf) ? buf : "?");
                                if (buf)
                                    free (buf);
                                if (ptr_next)
                                    ptr_next[0] = saved_char;
                                ptr_buf = ptr_next;
                                //if (!ptr_buf || !ptr_buf[0])
                                //    wattroff (GUI_CURSES(ptr_win)->win_nick,
                                //              A_UNDERLINE);
                            }
                            else
                                wprintw (GUI_CURSES(ptr_win)->win_nick, " ");
                        }
                    }
                    y++;
                    if ((CONFIG_INTEGER(config_look_nicklist_position) == CONFIG_LOOK_NICKLIST_TOP) ||
                        (CONFIG_INTEGER(config_look_nicklist_position) == CONFIG_LOOK_NICKLIST_BOTTOM))
                    {
                        if (y >= max_y)
                        {
                            column += max_length + 2;
                            y = (CONFIG_BOOLEAN(config_look_nicklist_separator)
                                 && (CONFIG_INTEGER(config_look_nicklist_position) == CONFIG_LOOK_NICKLIST_BOTTOM)) ?
                                1 : 0;
                        }
                    }
                }
                gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
            }
            wnoutrefresh (GUI_CURSES(ptr_win)->win_nick);
            refresh ();
        }
    }
}
