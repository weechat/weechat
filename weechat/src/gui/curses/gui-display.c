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

/* gui-display.c: display functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <curses.h>

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../common/weeconfig.h"
#include "../../irc/irc.h"


t_gui_color gui_colors[] =
{ { "default", -1 | A_NORMAL },
  { "black", COLOR_BLACK | A_NORMAL },
  { "red", COLOR_RED | A_NORMAL },
  { "lightred", COLOR_RED | A_BOLD },
  { "green", COLOR_GREEN | A_NORMAL },
  { "lightgreen", COLOR_GREEN | A_BOLD },
  { "brown", COLOR_YELLOW | A_NORMAL },
  { "yellow", COLOR_YELLOW | A_BOLD },
  { "blue", COLOR_BLUE | A_NORMAL },
  { "lightblue", COLOR_BLUE | A_BOLD },
  { "magenta", COLOR_MAGENTA | A_NORMAL },
  { "lightmagenta", COLOR_MAGENTA | A_BOLD },
  { "cyan", COLOR_CYAN | A_NORMAL },
  { "lightcyan", COLOR_CYAN | A_BOLD },
  { "gray", COLOR_WHITE },
  { "white", COLOR_WHITE | A_BOLD },
  { NULL, 0 }
};

char *nicks_colors[COLOR_WIN_NICK_NUMBER] =
{ "cyan", "magenta", "green", "brown", "lightblue", "gray",
  "lightcyan", "lightmagenta", "lightgreen", "blue" };

int color_attr[NUM_COLORS];

/*
 * gui_assign_color: assign a color (read from config)
 */

int
gui_assign_color (int *color, char *color_name)
{
    int i;
    
    /* look for curses colors in table */
    i = 0;
    while (gui_colors[i].name)
    {
        if (strcasecmp (gui_colors[i].name, color_name) == 0)
        {
            *color = gui_colors[i].color;
            return 1;
        }
        i++;
    }
    
    /* color not found */
    return 0;
}

/*
 * gui_get_color_by_name: get color by name
 */

int
gui_get_color_by_name (char *color_name)
{
    int i;
    
    /* look for curses colors in table */
    i = 0;
    while (gui_colors[i].name)
    {
        if (strcasecmp (gui_colors[i].name, color_name) == 0)
            return gui_colors[i].color;
        i++;
    }
    
    /* color not found */
    return -1;
}

/*
 * gui_get_color_by_value: get color name by value
 */

char *
gui_get_color_by_value (int color_value)
{
    int i;
    
    /* look for curses colors in table */
    i = 0;
    while (gui_colors[i].name)
    {
        if (gui_colors[i].color == color_value)
            return gui_colors[i].name;
        i++;
    }
    
    /* color not found */
    return NULL;
}

/*
 * gui_window_set_color: set color for window
 */

void
gui_window_set_color (WINDOW *window, int num_color)
{
    if (has_colors)
    {
        if (color_attr[num_color - 1] & A_BOLD)
            wattron (window, COLOR_PAIR (num_color) | A_BOLD);
        else
        {
            wattroff (window, A_BOLD);
            wattron (window, COLOR_PAIR (num_color));
        }
    }
}

/*
 * gui_window_has_nicklist: returns 1 if window has nicklist
 */

int
gui_window_has_nicklist (t_gui_window *window)
{
    return (window->win_nick != NULL);
}


/*
 * gui_calculate_pos_size: calculate position and size for a window & sub-win
 */

void
gui_calculate_pos_size (t_gui_window *window)
{
    int max_length, lines;
    int num_nicks, num_op, num_halfop, num_voice, num_normal;
    
    /* global position & size */
    /* TODO: get values from function parameters */
    window->win_x = 0;
    window->win_y = 0;
    window->win_width = COLS;
    window->win_height = LINES;
    
    /* init chat & nicklist settings */
    /* TODO: calculate values from function parameters */
    if (WIN_IS_CHANNEL(window))
    {
        max_length = nick_get_max_length (CHANNEL(window));
        
        switch (cfg_look_nicklist_position)
        {
            case CFG_LOOK_NICKLIST_LEFT:
                window->win_chat_x = max_length + 2;
                window->win_chat_y = 1;
                window->win_chat_width = COLS - max_length - 2;
                window->win_chat_height = LINES - 3;
                window->win_nick_x = 0;
                window->win_nick_y = 1;
                window->win_nick_width = max_length + 2;
                window->win_nick_height = LINES - 3;
                break;
            case CFG_LOOK_NICKLIST_RIGHT:
                window->win_chat_x = 0;
                window->win_chat_y = 1;
                window->win_chat_width = COLS - max_length - 2;
                window->win_chat_height = LINES - 3;
                window->win_nick_x = COLS - max_length - 2;
                window->win_nick_y = 1;
                window->win_nick_width = max_length + 2;
                window->win_nick_height = LINES - 3;
                break;
            case CFG_LOOK_NICKLIST_TOP:
                nick_count (CHANNEL(window), &num_nicks, &num_op, &num_halfop,
                            &num_voice, &num_normal);
                if (((max_length + 1) * num_nicks) % COLS == 0)
                    lines = ((max_length + 1) * num_nicks) / COLS;
                else
                    lines = (((max_length + 1) * num_nicks) / COLS) + 1;
                window->win_chat_x = 0;
                window->win_chat_y = 1 + (lines + 1);
                window->win_chat_width = COLS;
                window->win_chat_height = LINES - 3 - (lines + 1);
                window->win_nick_x = 0;
                window->win_nick_y = 1;
                window->win_nick_width = COLS;
                window->win_nick_height = lines + 1;
                break;
            case CFG_LOOK_NICKLIST_BOTTOM:
                nick_count (CHANNEL(window), &num_nicks, &num_op, &num_halfop,
                            &num_voice, &num_normal);
                if (((max_length + 1) * num_nicks) % COLS == 0)
                    lines = ((max_length + 1) * num_nicks) / COLS;
                else
                    lines = (((max_length + 1) * num_nicks) / COLS) + 1;
                window->win_chat_x = 0;
                window->win_chat_y = 1;
                window->win_chat_width = COLS;
                window->win_chat_height = LINES - 3 - (lines + 1);
                window->win_nick_x = 0;
                window->win_nick_y = LINES - 2 - (lines + 1);
                window->win_nick_width = COLS;
                window->win_nick_height = lines + 1;
                break;
        }
        
        window->win_chat_cursor_x = 0;
        window->win_chat_cursor_y = 0;
    }
    else
    {
        window->win_chat_x = 0;
        window->win_chat_y = 1;
        window->win_chat_width = COLS;
        window->win_chat_height = LINES - 3;
        window->win_chat_cursor_x = 0;
        window->win_chat_cursor_y = 0;
        window->win_nick_x = -1;
        window->win_nick_y = -1;
        window->win_nick_width = -1;
        window->win_nick_height = -1;
    }
}

/*
 * gui_curses_window_clear: clear a window
 */

void
gui_curses_window_clear (WINDOW *window)
{
    werase (window);
    wmove (window, 0, 0);
    //wrefresh (window);
}

/*
 * gui_draw_window_title: draw title window
 */

void
gui_draw_window_title (t_gui_window *window)
{
    char format[32];
    
    /* TODO: manage splitted windows! */
    if (window != gui_current_window)
        return;
    
    if (has_colors ())
    {
        gui_window_set_color (window->win_title, COLOR_WIN_TITLE);
        wborder (window->win_title, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        wrefresh (window->win_title);
        refresh ();
    }
    if (CHANNEL(window))
    {
        sprintf (format, "%%-%ds", window->win_width);
        if (CHANNEL(window)->topic)
            mvwprintw (window->win_title, 0, 0, format,
                       CHANNEL(window)->topic);
    }
    else
    {
        /* TODO: change this copyright as title? */
        mvwprintw (window->win_title, 0, 0,
                   "%s", PACKAGE_STRING " - " WEECHAT_WEBSITE);
        mvwprintw (window->win_title, 0, COLS - strlen (WEECHAT_COPYRIGHT),
                   "%s", WEECHAT_COPYRIGHT);
    }
    wrefresh (window->win_title);
    refresh ();
}

/*
 * gui_redraw_window_title: redraw title window
 */

void
gui_redraw_window_title (t_gui_window *window)
{
    /* TODO: manage splitted windows! */
    if (window != gui_current_window)
        return;
    
    gui_curses_window_clear (window->win_title);
    gui_draw_window_title (window);
}

/*
 * gui_get_line_num_splits: returns number of lines on window
 *                          (depending on window width and type (server/channel)
 *                          for alignment)
 */

int
gui_get_line_num_splits (t_gui_window *window, t_gui_line *line)
{
    int length, width;
    
    /* TODO: modify arbitraty value for non aligning messages on time/nick? */
    if (line->length_align >= window->win_chat_width - 5)
    {
        length = line->length;
        width = window->win_chat_width;
    }
    else
    {
        length = line->length - line->length_align;
        width = window->win_chat_width - line->length_align;
    }
    
    return (length % width == 0) ? (length / width) : ((length / width) + 1);
}

/*
 * gui_display_end_of_line: display end of a line in the chat window
 */

void
gui_display_end_of_line (t_gui_window *window, t_gui_line *line, int count)
{
    int lines_displayed, num_lines, offset, remainder, num_displayed;
    t_gui_message *ptr_message;
    char saved_char, format_align[32];
    
    sprintf (format_align, "%%-%ds", line->length_align);
    num_lines = gui_get_line_num_splits (window, line);
    ptr_message = line->messages;
    offset = 0;
    lines_displayed = 0;
    while (ptr_message)
    {
        /* set text color if beginning of message */
        if (offset == 0)
            gui_window_set_color (window->win_chat, ptr_message->color);
        
        /* insert spaces for align text under time/nick */
        if ((lines_displayed > 0) && (window->win_chat_cursor_x == 0))
        {
            if (lines_displayed >= num_lines - count)
                mvwprintw (window->win_chat,
                           window->win_chat_cursor_y,
                           window->win_chat_cursor_x,
                           format_align, " ");
            window->win_chat_cursor_x += line->length_align;
        }
        
        remainder = strlen (ptr_message->message + offset);
        if (window->win_chat_cursor_x + remainder >
            window->win_chat_width - 1)
        {
            num_displayed = window->win_chat_width -
                window->win_chat_cursor_x;
            saved_char = ptr_message->message[offset + num_displayed];
            ptr_message->message[offset + num_displayed] = '\0';
            if (lines_displayed >= num_lines - count)
                mvwprintw (window->win_chat,
                           window->win_chat_cursor_y, 
                           window->win_chat_cursor_x,
                           "%s", ptr_message->message + offset);
            ptr_message->message[offset + num_displayed] = saved_char;
            offset += num_displayed;
        }
        else
        {
            num_displayed = remainder;
            if (lines_displayed >= num_lines - count)
                mvwprintw (window->win_chat,
                           window->win_chat_cursor_y, 
                           window->win_chat_cursor_x,
                           "%s", ptr_message->message + offset);
            ptr_message = ptr_message->next_message;
            offset = 0;
        }
        window->win_chat_cursor_x += num_displayed;
        if (!ptr_message ||
            (window->win_chat_cursor_x > (window->win_chat_width - 1)))
        {
            if (lines_displayed >= num_lines - count)
            {
                if (window->win_chat_cursor_x <= window->win_chat_width - 1)
                    wclrtoeol (window->win_chat);
                window->win_chat_cursor_y++;
            }
            window->win_chat_cursor_x = 0;
            lines_displayed++;
        }
    }
}

/*
 * gui_display_line: display a line in the chat window
 *                   if stop_at_end == 1, screen will not scroll and then we
 *                   exit since chat window is full
 *                   returns: 1 if stop_at_end == 0 or screen not full
 *                            0 if screen is full and if stop_at_end == 1
 */

int
gui_display_line (t_gui_window *window, t_gui_line *line, int stop_at_end)
{
    int offset, remainder, num_displayed;
    t_gui_message *ptr_message;
    char saved_char, format_align[32];
    
    sprintf (format_align, "%%-%ds", line->length_align);
    ptr_message = line->messages;
    offset = 0;
    while (ptr_message)
    {
        /* cursor is below end line of chat window */
        if (window->win_chat_cursor_y > window->win_chat_height - 1)
        {
            /*if (!stop_at_end)
                wscrl (window->win_chat, +1);*/
            window->win_chat_cursor_x = 0;
            window->win_chat_cursor_y = window->win_chat_height - 1;
            if (stop_at_end)
                return 0;
            window->first_line_displayed = 0;
        }
        
        /* set text color if beginning of message */
        if (offset == 0)
            gui_window_set_color (window->win_chat, ptr_message->color);
        
        /* insert spaces for align text under time/nick */
        if ((window->win_chat_cursor_x == 0) &&
            (ptr_message->type != MSG_TYPE_TIME) &&
            (ptr_message->type != MSG_TYPE_NICK) &&
            (line->length_align > 0) &&
            /* TODO: modify arbitraty value for non aligning messages on time/nick? */
            (line->length_align < (window->win_chat_width - 5)))
        {
            mvwprintw (window->win_chat,
                       window->win_chat_cursor_y, 
                       window->win_chat_cursor_x,
                       format_align, " ");
            window->win_chat_cursor_x += line->length_align;
        }
        
        remainder = strlen (ptr_message->message + offset);
        if (window->win_chat_cursor_x + remainder > window->win_chat_width)
        {
            num_displayed = window->win_chat_width -
                window->win_chat_cursor_x;
            saved_char = ptr_message->message[offset + num_displayed];
            ptr_message->message[offset + num_displayed] = '\0';
            mvwprintw (window->win_chat,
                       window->win_chat_cursor_y, 
                       window->win_chat_cursor_x,
                       "%s", ptr_message->message + offset);
            ptr_message->message[offset + num_displayed] = saved_char;
            offset += num_displayed;
        }
        else
        {
            num_displayed = remainder;
            mvwprintw (window->win_chat,
                       window->win_chat_cursor_y, 
                       window->win_chat_cursor_x,
                       "%s", ptr_message->message + offset);
            offset = 0;
            ptr_message = ptr_message->next_message;
        }
        window->win_chat_cursor_x += num_displayed;
        if (!ptr_message ||
            (window->win_chat_cursor_x > (window->win_chat_width - 1)))
        {
            if (!ptr_message ||
                ((window->win_chat_cursor_y <= window->win_chat_height - 1) &&
                (window->win_chat_cursor_x > window->win_chat_width - 1)))
            {
                if (window->win_chat_cursor_x <= window->win_chat_width - 1)
                    wclrtoeol (window->win_chat);
                window->win_chat_cursor_y++;
            }
            window->win_chat_cursor_x = 0;
        }
    }
    return 1;
}

/*
 * gui_draw_window_chat: draw chat window
 */

void
gui_draw_window_chat (t_gui_window *window)
{
    t_gui_line *ptr_line;
    int lines_used;
    
    /* TODO: manage splitted windows! */
    if (window != gui_current_window)
        return;
    
    if (has_colors ())
    {
        gui_window_set_color (window->win_chat, COLOR_WIN_CHAT);
        wborder (window->win_chat, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        wrefresh (window->win_chat);
    }
    
    ptr_line = window->last_line;
    lines_used = 0;
    while (ptr_line
           && (lines_used < (window->win_chat_height + window->sub_lines)))
    {
        lines_used += gui_get_line_num_splits (window, ptr_line);
        ptr_line = ptr_line->prev_line;
    }
    window->win_chat_cursor_x = 0;
    window->win_chat_cursor_y = 0;
    if (lines_used > (window->win_chat_height + window->sub_lines))
    {
        /* screen will be full (we'll display only end of 1st line) */
        ptr_line = (ptr_line) ? ptr_line->next_line : window->lines;
        gui_display_end_of_line (window, ptr_line,
                                 gui_get_line_num_splits (window, ptr_line) -
                                 (lines_used - (window->win_chat_height + window->sub_lines)));
        ptr_line = ptr_line->next_line;
        window->first_line_displayed = 0;
    }
    else
    {
        /* all lines are displayed */
        if (!ptr_line)
        {
            window->first_line_displayed = 1;
            ptr_line = window->lines;
        }
        else
        {
            window->first_line_displayed = 0;
            ptr_line = ptr_line->next_line;
        }
    }
    while (ptr_line)
    {
        if (!gui_display_line (window, ptr_line, 1))
            break;
  
        ptr_line = ptr_line->next_line;
    }
    /*if (window->win_chat_cursor_y <= window->win_chat_height - 1)
        window->sub_lines = 0;*/
    wrefresh (window->win_chat);
    refresh ();
}

/*
 * gui_redraw_window_chat: redraw chat window
 */

void
gui_redraw_window_chat (t_gui_window *window)
{
    /* TODO: manage splitted windows! */
    if (window != gui_current_window)
        return;
    
    gui_curses_window_clear (window->win_chat);
    gui_draw_window_chat (window);
}

/*
 * gui_draw_window_nick: draw nick window
 */

void
gui_draw_window_nick (t_gui_window *window)
{
    int i, x, y, column, max_length;
    char format[32];
    t_irc_nick *ptr_nick;
    
    /* TODO: manage splitted windows! */
    if (window != gui_current_window)
        return;
    
    if (CHANNEL(window) && CHANNEL(window)->nicks)
    {
        max_length = nick_get_max_length (CHANNEL(window));
        if ((window == gui_current_window) &&
            ((max_length + 2) != window->win_nick_width))
        {
            gui_calculate_pos_size (window);
            delwin (window->win_chat);
            delwin (window->win_nick);
            window->win_chat = newwin (window->win_chat_height,
                                       window->win_chat_width,
                                       window->win_chat_y,
                                       window->win_chat_x);
            window->win_nick = newwin (window->win_nick_height,
                                       window->win_nick_width,
                                       window->win_nick_y,
                                       window->win_nick_x);
            gui_draw_window_chat (window);
        }
        sprintf (format, "%%-%ds", max_length);
            
        if (has_colors ())
        {
            switch (cfg_look_nicklist_position)
            {
                case CFG_LOOK_NICKLIST_LEFT:
                    gui_window_set_color (window->win_nick, COLOR_WIN_NICK_SEP);
                    for (i = 0; i < window->win_chat_height; i++)
                        mvwprintw (window->win_nick,
                                   i, window->win_nick_width - 1, " ");
                    break;
                case CFG_LOOK_NICKLIST_RIGHT:
                    gui_window_set_color (window->win_nick, COLOR_WIN_NICK_SEP);
                    for (i = 0; i < window->win_chat_height; i++)
                        mvwprintw (window->win_nick,
                                   i, 0, " ");
                    break;
                case CFG_LOOK_NICKLIST_TOP:
                    gui_window_set_color (window->win_nick, COLOR_WIN_NICK);
                    for (i = 0; i < window->win_chat_width; i += 2)
                        mvwprintw (window->win_nick,
                                   window->win_nick_height - 1, i, "-");
                    break;
                case CFG_LOOK_NICKLIST_BOTTOM:
                    gui_window_set_color (window->win_nick, COLOR_WIN_NICK);
                    for (i = 0; i < window->win_chat_width; i += 2)
                        mvwprintw (window->win_nick,
                                   0, i, "-");
                    break;
            }
        }
    
        gui_window_set_color (window->win_nick, COLOR_WIN_NICK);
        x = 0;
        y = (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_BOTTOM) ? 1 : 0;
        column = 0;
        for (ptr_nick = CHANNEL(window)->nicks; ptr_nick;
             ptr_nick = ptr_nick->next_nick)
        {
            switch (cfg_look_nicklist_position)
            {
                case CFG_LOOK_NICKLIST_LEFT:
                    x = 0;
                    break;
                case CFG_LOOK_NICKLIST_RIGHT:
                    x = 1;
                    break;
                case CFG_LOOK_NICKLIST_TOP:
                case CFG_LOOK_NICKLIST_BOTTOM:
                    x = column;
                    break;
            }
            if (ptr_nick->is_op)
            {
                gui_window_set_color (window->win_nick, COLOR_WIN_NICK_OP);
                mvwprintw (window->win_nick, y, x, "@");
                x++;
            }
            else
            {
                if (ptr_nick->is_halfop)
                {
                    gui_window_set_color (window->win_nick, COLOR_WIN_NICK_HALFOP);
                    mvwprintw (window->win_nick, y, x, "%%");
                    x++;
                }
                else
                {
                    if (ptr_nick->has_voice)
                    {
                        gui_window_set_color (window->win_nick, COLOR_WIN_NICK_VOICE);
                        mvwprintw (window->win_nick, y, x, "+");
                        x++;
                    }
                    else
                    {
                        gui_window_set_color (window->win_nick, COLOR_WIN_NICK);
                        mvwprintw (window->win_nick, y, x, " ");
                        x++;
                    }
                }
            }
            gui_window_set_color (window->win_nick, COLOR_WIN_NICK);
            mvwprintw (window->win_nick, y, x, format, ptr_nick->nick);
            y++;
            if ((cfg_look_nicklist_position == CFG_LOOK_NICKLIST_TOP) ||
                (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_BOTTOM))
            {
                if (y >= window->win_nick_height - 1)
                {
                    column += max_length + 1;
                    y = (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_TOP) ?
                        0 : 1;
                }
            }
        }
    }
    wrefresh (window->win_nick);
    refresh ();
}

/*
 * gui_redraw_window_nick: redraw nick window
 */

void
gui_redraw_window_nick (t_gui_window *window)
{
    /* TODO: manage splitted windows! */
    if (window != gui_current_window)
        return;
    
    gui_curses_window_clear (window->win_nick);
    gui_draw_window_nick (window);
}

/*
 * gui_draw_window_status: draw status window
 */

void
gui_draw_window_status (t_gui_window *window)
{
    t_gui_window *ptr_win;
    char format_more[32];
    
    /* TODO: manage splitted windows! */
    if (window != gui_current_window)
        return;
    
    if (has_colors ())
    {
        gui_window_set_color (window->win_status, COLOR_WIN_STATUS);
        wborder (window->win_status, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        wrefresh (window->win_status);
    }
    //refresh ();
    wmove (window->win_status, 0, 0);
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (SERVER(ptr_win) && !CHANNEL(ptr_win))
        {
            if (gui_current_window == SERVER(ptr_win)->window)
            {
                if (ptr_win->unread_data)
                {
                    if (ptr_win->unread_data > 1)
                        gui_window_set_color (window->win_status,
                                              COLOR_WIN_STATUS_DATA_MSG);
                    else
                        gui_window_set_color (window->win_status,
                                              COLOR_WIN_STATUS_DATA_OTHER);
                }
                else
                    gui_window_set_color (window->win_status,
                                          COLOR_WIN_STATUS_ACTIVE);
            }
            else
            {
                if (SERVER(ptr_win)->window &&
                    ((SERVER(ptr_win)->window)->unread_data))
                {
                    if (SERVER(ptr_win)->window->unread_data > 1)
                        gui_window_set_color (window->win_status,
                                              COLOR_WIN_STATUS_DATA_MSG);
                    else
                        gui_window_set_color (window->win_status,
                                              COLOR_WIN_STATUS_DATA_OTHER);
                }
                else
                    gui_window_set_color (window->win_status,
                                          COLOR_WIN_STATUS);
            }
            if (SERVER(ptr_win)->is_connected)
                wprintw (window->win_status, "[%s] ",
                         SERVER(ptr_win)->name);
            else
                wprintw (window->win_status, "(%s) ",
                         SERVER(ptr_win)->name);
        }
        if (SERVER(ptr_win) && CHANNEL(ptr_win))
        {
            if (gui_current_window == CHANNEL(ptr_win)->window)
            {
                if ((CHANNEL(ptr_win)->window) &&
                    (CHANNEL(ptr_win)->window->unread_data))
                {
                    if (CHANNEL(ptr_win)->window->unread_data > 1)
                        gui_window_set_color (window->win_status,
                                              COLOR_WIN_STATUS_DATA_MSG);
                    else
                        gui_window_set_color (window->win_status,
                                              COLOR_WIN_STATUS_DATA_OTHER);
                }
                else
                    gui_window_set_color (window->win_status,
                                          COLOR_WIN_STATUS_ACTIVE);
            }
            else
            {
                if ((CHANNEL(ptr_win)->window) &&
                    (CHANNEL(ptr_win)->window->unread_data))
                {
                    if (CHANNEL(ptr_win)->window->unread_data > 1)
                        gui_window_set_color (window->win_status,
                                              COLOR_WIN_STATUS_DATA_MSG);
                    else
                        gui_window_set_color (window->win_status,
                                              COLOR_WIN_STATUS_DATA_OTHER);
                }
                else
                    gui_window_set_color (window->win_status,
                                          COLOR_WIN_STATUS);
            }
            wprintw (window->win_status, "%s ", CHANNEL(ptr_win)->name);
        }
        if (!SERVER(ptr_win))
        {
            gui_window_set_color (window->win_status, COLOR_WIN_STATUS);
            wprintw (window->win_status, _("[not connected] "));
        }
    }
    
    /* display "*MORE*" if last line is not displayed */
    gui_window_set_color (window->win_status, COLOR_WIN_STATUS_MORE);
    if (window->sub_lines > 0)
        mvwprintw (window->win_status, 0, COLS - 7, _("-MORE-"));
    else
    {
        sprintf (format_more, "%%-%ds", strlen (_("-MORE-")));
        mvwprintw (window->win_status, 0, COLS - 7, format_more, " ");
    }
    
    wrefresh (window->win_status);
    refresh ();
}

/*
 * gui_redraw_window_status: redraw status window
 */

void
gui_redraw_window_status (t_gui_window *window)
{
    /* TODO: manage splitted windows! */
    if (window != gui_current_window)
        return;
    
    gui_curses_window_clear (window->win_status);
    gui_draw_window_status (window);
}

/*
 * gui_get_input_width: return input width (max # chars displayed)
 */

int
gui_get_input_width (t_gui_window *window)
{
    if (CHANNEL(window))
        return (COLS - strlen (CHANNEL(window)->name) -
                strlen (SERVER(window)->nick) - 3);
    else
    {
        if (SERVER(window) && (SERVER(window)->is_connected))
            return (COLS - strlen (SERVER(window)->nick) - 2);
        else
            return (COLS - strlen (cfg_look_no_nickname) - 2);
    }
}

/*
 * gui_draw_window_input: draw input window
 */

void
gui_draw_window_input (t_gui_window *window)
{
    char format[32];
    char *ptr_nickname;
    int input_width;
    
    /* TODO: manage splitted windows! */
    if (window != gui_current_window)
        return;
    
    if (has_colors ())
    {
        gui_window_set_color (window->win_input, COLOR_WIN_INPUT);
        wborder (window->win_input, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        wrefresh (window->win_input);
    }
    //refresh ();
    
    if (window->input_buffer_size == 0)
        window->input_buffer[0] = '\0';
    
    input_width = gui_get_input_width (window);
    
    if (window->input_buffer_pos - window->input_buffer_1st_display + 1 >
        input_width)
        window->input_buffer_1st_display = window->input_buffer_pos -
            input_width + 1;
    else
    {
        if (window->input_buffer_pos < window->input_buffer_1st_display)
            window->input_buffer_1st_display = window->input_buffer_pos;
        else
        {
            if ((window->input_buffer_1st_display > 0) &&
                (window->input_buffer_pos -
                window->input_buffer_1st_display + 1) < input_width)
            {
                window->input_buffer_1st_display =
                    window->input_buffer_pos - input_width + 1;
                if (window->input_buffer_1st_display < 0)
                    window->input_buffer_1st_display = 0;
            }
        }
    }
    if (CHANNEL(window))
    {
        sprintf (format, "%%s %%s> %%-%ds", input_width);
        mvwprintw (window->win_input, 0, 0, format,
                   CHANNEL(window)->name,
                   SERVER(window)->nick,
                   window->input_buffer + window->input_buffer_1st_display);
        wclrtoeol (window->win_input);
        move (LINES - 1, strlen (CHANNEL(window)->name) +
              strlen (SERVER(window)->nick) + 3 +
              (window->input_buffer_pos - window->input_buffer_1st_display));
    }
    else
    {
        if (SERVER(window))
        {
            sprintf (format, "%%s> %%-%ds", input_width);
            if (SERVER(window) && (SERVER(window)->is_connected))
                ptr_nickname = SERVER(window)->nick;
            else
                ptr_nickname = cfg_look_no_nickname;
            mvwprintw (window->win_input, 0, 0, format,
                       ptr_nickname,
                       window->input_buffer + window->input_buffer_1st_display);
            wclrtoeol (window->win_input);
            move (LINES - 1, strlen (ptr_nickname) + 2 +
                  (window->input_buffer_pos - window->input_buffer_1st_display));
        }
        else
        {
            sprintf (format, "%%s> %%-%ds", input_width);
            if (SERVER(window) && (SERVER(window)->is_connected))
                ptr_nickname = SERVER(window)->nick;
            else
                ptr_nickname = cfg_look_no_nickname;
            mvwprintw (window->win_input, 0, 0, format,
                       ptr_nickname,
                       window->input_buffer + window->input_buffer_1st_display);
            wclrtoeol (window->win_input);
            move (LINES - 1, strlen (ptr_nickname) + 2 +
                  (window->input_buffer_pos - window->input_buffer_1st_display));
        }
    }
    
    wrefresh (window->win_input);
    refresh ();
}

/*
 * gui_redraw_window_input: redraw input window
 */

void
gui_redraw_window_input (t_gui_window *window)
{
    /* TODO: manage splitted windows! */
    if (window != gui_current_window)
        return;
    
    gui_curses_window_clear (window->win_input);
    gui_draw_window_input (window);
}

/*
 * gui_redraw_window: redraw a window
 */

void
gui_redraw_window (t_gui_window *window)
{
    /* TODO: manage splitted windows! */
    if (window != gui_current_window)
        return;
    
    gui_redraw_window_title (window);
    gui_redraw_window_chat (window);
    if (window->win_nick)
        gui_redraw_window_nick (window);
    gui_redraw_window_status (window);
    gui_redraw_window_input (window);
}

/*
 * gui_switch_to_window: switch to another window
 */

void
gui_switch_to_window (t_gui_window *window)
{
    int another_window;
    t_gui_window *ptr_win;
    
    another_window = 0;
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->is_displayed)
        {
            /* TODO: manage splitted windows */
            another_window = 1;
            window->win_title = ptr_win->win_title;
            window->win_chat = ptr_win->win_chat;
            window->win_nick = ptr_win->win_nick;
            window->win_status = ptr_win->win_status;
            window->win_input = ptr_win->win_input;
            ptr_win->win_title = NULL;
            ptr_win->win_chat = NULL;
            ptr_win->win_nick = NULL;
            ptr_win->win_status = NULL;
            ptr_win->win_input = NULL;
            ptr_win->is_displayed = 0;
            break;
        }
    }
    
    gui_calculate_pos_size (window);
    
    /* first time creation for windows */
    if (!another_window)
    {
        /* create new windows */
        window->win_title = newwin (1, COLS, 0, 0);
        window->win_chat = newwin (window->win_chat_height,
                                   window->win_chat_width,
                                   window->win_chat_y,
                                   window->win_chat_x);
        if (CHANNEL(window))
            window->win_nick = newwin (window->win_nick_height,
                                       window->win_nick_width,
                                       window->win_nick_y,
                                       window->win_nick_x);
        else
            window->win_nick = NULL;
        window->win_status = newwin (1, COLS, LINES - 2, 0);
        window->win_input = newwin (1, COLS, LINES - 1, 0);
    }
    else
    {
        /* create chat & nick windows */
        if (WIN_IS_CHANNEL(window))
        {
            /* (re)create nicklist window */
            if (window->win_nick)
                delwin (window->win_nick);
            delwin (window->win_chat);
            window->win_chat = newwin (window->win_chat_height,
                                       window->win_chat_width,
                                       window->win_chat_y,
                                       window->win_chat_x);
            window->win_nick = newwin (window->win_nick_height,
                                       window->win_nick_width,
                                       window->win_nick_y,
                                       window->win_nick_x);
        }
        if (!(WIN_IS_CHANNEL(window)))
        {
            /* remove nick list window */
            if (window->win_nick)
                delwin (window->win_nick);
            window->win_nick = NULL;
            delwin (window->win_chat);
            window->win_chat = newwin (window->win_chat_height,
                                       window->win_chat_width,
                                       window->win_chat_y,
                                       window->win_chat_x);
        }
    }
    
    /* change current window to the new window */
    gui_current_window = window;
    
    window->is_displayed = 1;
    window->unread_data = 0;
}

/*
 * gui_switch_to_previous_window: switch to previous window
 */

void
gui_switch_to_previous_window ()
{
    /* if only one windows then return */
    if (gui_windows == last_gui_window)
        return;
    
    if (gui_current_window->prev_window)
        gui_switch_to_window (gui_current_window->prev_window);
    else
        gui_switch_to_window (last_gui_window);
    gui_redraw_window (gui_current_window);
}

/*
 * gui_switch_to_next_window: switch to next window
 */

void
gui_switch_to_next_window ()
{
    /* if only one windows then return */
    if (gui_windows == last_gui_window)
        return;
    
    if (gui_current_window->next_window)
        gui_switch_to_window (gui_current_window->next_window);
    else
        gui_switch_to_window (gui_windows);
    gui_redraw_window (gui_current_window);
}

/*
 * gui_move_page_up: display previous page on window
 */

void
gui_move_page_up ()
{
    if (!gui_current_window->first_line_displayed)
    {
        gui_current_window->sub_lines += gui_current_window->win_chat_height - 1;
        gui_redraw_window_chat (gui_current_window);
        gui_redraw_window_status (gui_current_window);
    }
}

/*
 * gui_move_page_down: display next page on window
 */

void
gui_move_page_down ()
{
    if (gui_current_window->sub_lines > 0)
    {
        gui_current_window->sub_lines -= gui_current_window->win_chat_height - 1;
        if (gui_current_window->sub_lines < 0)
            gui_current_window->sub_lines = 0;
        if (gui_current_window->sub_lines == 0)
            gui_current_window->unread_data = 0;
        gui_redraw_window_chat (gui_current_window);
        gui_redraw_window_status (gui_current_window);
    }
}

/*
 * gui_curses_resize_handler: called when term size is modified
 */

void
gui_curses_resize_handler ()
{
    t_gui_window *ptr_win;
    int width, height;
    
    endwin ();
    refresh ();
    
    getmaxyx (stdscr, height, width);
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        gui_calculate_pos_size (ptr_win);
        // TODO: manage splitted windows!
        if (ptr_win->win_title)
        {
            if (ptr_win->win_title)
                delwin (ptr_win->win_title);
            if (ptr_win->win_chat)
                delwin (ptr_win->win_chat);
            if (ptr_win->win_nick)
                delwin (ptr_win->win_nick);
            if (ptr_win->win_status)
                delwin (ptr_win->win_status);
            if (ptr_win->win_input)
                delwin (ptr_win->win_input);
            ptr_win->win_title = NULL;
            ptr_win->win_chat = NULL;
            ptr_win->win_nick = NULL;
            ptr_win->win_status = NULL;
            ptr_win->win_input = NULL;
            gui_switch_to_window (ptr_win);
        }
    }
}

/*
 * gui_window_init_subwindows: init subwindows for a WeeChat window
 */

void
gui_window_init_subwindows (t_gui_window *window)
{
    window->win_title = NULL;
    window->win_chat = NULL;
    window->win_nick = NULL;
    window->win_status = NULL;
    window->win_input = NULL;
}

/*
 * gui_pre_init: pre-initialize GUI (called before gui_init)
 */

void
gui_pre_init (int *argc, char **argv[])
{
    /* nothing for Curses interface */
    (void) argc;
    (void) argv;
}

/*
 * gui_init_colors: init GUI colors
 */

void
gui_init_colors ()
{
    int i, color;
    
    if (has_colors ())
    {
        start_color ();
        use_default_colors ();
        
        init_pair (COLOR_WIN_TITLE,
            cfg_col_title & A_CHARTEXT, cfg_col_title_bg);
        init_pair (COLOR_WIN_CHAT,
            cfg_col_chat & A_CHARTEXT, cfg_col_chat_bg);
        init_pair (COLOR_WIN_CHAT_TIME,
            cfg_col_chat_time & A_CHARTEXT, cfg_col_chat_bg);
        init_pair (COLOR_WIN_CHAT_TIME_SEP,
            cfg_col_chat_time_sep & A_CHARTEXT, cfg_col_chat_bg);
        init_pair (COLOR_WIN_CHAT_PREFIX1,
            cfg_col_chat_prefix1 & A_CHARTEXT, cfg_col_chat_bg);
        init_pair (COLOR_WIN_CHAT_PREFIX2,
            cfg_col_chat_prefix2 & A_CHARTEXT, cfg_col_chat_bg);
        init_pair (COLOR_WIN_CHAT_NICK,
            cfg_col_chat_nick & A_CHARTEXT, cfg_col_chat_bg);
        init_pair (COLOR_WIN_CHAT_HOST,
            cfg_col_chat_host & A_CHARTEXT, cfg_col_chat_bg);
        init_pair (COLOR_WIN_CHAT_CHANNEL,
            cfg_col_chat_channel & A_CHARTEXT, cfg_col_chat_bg);
        init_pair (COLOR_WIN_CHAT_DARK,
            cfg_col_chat_dark & A_CHARTEXT, cfg_col_chat_bg);
        init_pair (COLOR_WIN_STATUS,
            cfg_col_status & A_CHARTEXT, cfg_col_status_bg);
        init_pair (COLOR_WIN_STATUS_ACTIVE,
            cfg_col_status_active & A_CHARTEXT, cfg_col_status_bg);
        init_pair (COLOR_WIN_STATUS_DATA_MSG,
            cfg_col_status_data_msg & A_CHARTEXT, cfg_col_status_bg);
        init_pair (COLOR_WIN_STATUS_DATA_OTHER,
            cfg_col_status_data_other & A_CHARTEXT, cfg_col_status_bg);
        init_pair (COLOR_WIN_STATUS_MORE,
            cfg_col_status_more & A_CHARTEXT, cfg_col_status_bg);
        init_pair (COLOR_WIN_INPUT,
            cfg_col_input & A_CHARTEXT, cfg_col_input_bg);
        init_pair (COLOR_WIN_INPUT_CHANNEL,
            cfg_col_input_channel & A_CHARTEXT, cfg_col_input_bg);
        init_pair (COLOR_WIN_INPUT_NICK,
            cfg_col_input_nick & A_CHARTEXT, cfg_col_input_bg);
        init_pair (COLOR_WIN_NICK,
            cfg_col_nick & A_CHARTEXT, cfg_col_nick_bg);
        init_pair (COLOR_WIN_NICK_OP,
            cfg_col_nick_op & A_CHARTEXT, cfg_col_nick_bg);
        init_pair (COLOR_WIN_NICK_HALFOP,
            cfg_col_nick_halfop & A_CHARTEXT, cfg_col_nick_bg);
        init_pair (COLOR_WIN_NICK_VOICE,
            cfg_col_nick_voice & A_CHARTEXT, cfg_col_nick_bg);
        init_pair (COLOR_WIN_NICK_SEP,
            COLOR_BLACK & A_CHARTEXT, cfg_col_nick_sep);
        init_pair (COLOR_WIN_NICK_SELF,
            cfg_col_nick_self & A_CHARTEXT, cfg_col_nick_bg);
        init_pair (COLOR_WIN_NICK_PRIVATE,
            cfg_col_nick_private & A_CHARTEXT, cfg_col_nick_bg);
        
        for (i = 0; i < COLOR_WIN_NICK_NUMBER; i++)
        {
            gui_assign_color (&color, nicks_colors[i]);
            init_pair (COLOR_WIN_NICK_FIRST + i, color & A_CHARTEXT, cfg_col_chat_bg);
            color_attr[COLOR_WIN_NICK_FIRST + i - 1] =
                (color & A_BOLD) ? A_BOLD : 0;
        }
            
        color_attr[COLOR_WIN_TITLE - 1] = cfg_col_title & A_BOLD;
        color_attr[COLOR_WIN_CHAT - 1] = cfg_col_chat & A_BOLD;
        color_attr[COLOR_WIN_CHAT_TIME - 1] = cfg_col_chat_time & A_BOLD;
        color_attr[COLOR_WIN_CHAT_TIME_SEP - 1] = cfg_col_chat_time_sep & A_BOLD;
        color_attr[COLOR_WIN_CHAT_DARK - 1] = cfg_col_chat_dark & A_BOLD;
        color_attr[COLOR_WIN_CHAT_PREFIX1 - 1] = cfg_col_chat_prefix1 & A_BOLD;
        color_attr[COLOR_WIN_CHAT_PREFIX2 - 1] = cfg_col_chat_prefix2 & A_BOLD;
        color_attr[COLOR_WIN_CHAT_NICK - 1] = cfg_col_chat_nick & A_BOLD;
        color_attr[COLOR_WIN_CHAT_HOST - 1] = cfg_col_chat_host & A_BOLD;
        color_attr[COLOR_WIN_CHAT_CHANNEL - 1] = cfg_col_chat_channel & A_BOLD;
        color_attr[COLOR_WIN_CHAT_DARK - 1] = cfg_col_chat_dark & A_BOLD;
        color_attr[COLOR_WIN_STATUS - 1] = cfg_col_status & A_BOLD;
        color_attr[COLOR_WIN_STATUS_ACTIVE - 1] = cfg_col_status_active & A_BOLD;
        color_attr[COLOR_WIN_STATUS_DATA_MSG - 1] = cfg_col_status_data_msg & A_BOLD;
        color_attr[COLOR_WIN_STATUS_DATA_OTHER - 1] = cfg_col_status_data_other & A_BOLD;
        color_attr[COLOR_WIN_STATUS_MORE - 1] = cfg_col_status_more & A_BOLD;
        color_attr[COLOR_WIN_INPUT - 1] = cfg_col_input & A_BOLD;
        color_attr[COLOR_WIN_INPUT_CHANNEL - 1] = cfg_col_input_channel & A_BOLD;
        color_attr[COLOR_WIN_INPUT_NICK - 1] = cfg_col_input_nick & A_BOLD;
        color_attr[COLOR_WIN_NICK - 1] = cfg_col_nick & A_BOLD;
        color_attr[COLOR_WIN_NICK_OP - 1] = cfg_col_nick_op & A_BOLD;
        color_attr[COLOR_WIN_NICK_HALFOP - 1] = cfg_col_nick_halfop & A_BOLD;
        color_attr[COLOR_WIN_NICK_VOICE - 1] = cfg_col_nick_voice & A_BOLD;
        color_attr[COLOR_WIN_NICK_SEP - 1] = 0;
        color_attr[COLOR_WIN_NICK_SELF - 1] = cfg_col_nick_self & A_BOLD;
        color_attr[COLOR_WIN_NICK_PRIVATE - 1] = cfg_col_nick_private & A_BOLD;
    }
}

/*
 * gui_init: init GUI
 */

void
gui_init ()
{
    initscr ();
    
    curs_set (1);
    keypad (stdscr, TRUE);
    noecho ();
    /*nonl();*/
    nodelay (stdscr, TRUE);

    gui_init_colors ();

    /* create a new window */
    gui_current_window = gui_window_new (NULL, NULL /*0, 0, COLS, LINES*/);
    
    signal (SIGWINCH, gui_curses_resize_handler);
    
    #ifdef __linux__
    /* set title for term window, not for console */
    if (cfg_look_set_title && (strcmp (getenv ("TERM"), "linux") != 0))
        printf ("\e]2;" PACKAGE_NAME " " PACKAGE_VERSION "\a\e]1;" PACKAGE_NAME " " PACKAGE_VERSION "\a");
    #endif
    
    gui_ready = 1;
}

/*
 * gui_end: GUI end
 */

void
gui_end ()
{
    t_gui_window *ptr_win;
    
    /* delete all windows */
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->win_title)
            delwin (ptr_win->win_title);
        if (ptr_win->win_chat)
            delwin (ptr_win->win_chat);
        if (ptr_win->win_nick)
            delwin (ptr_win->win_nick);
        if (ptr_win->win_status)
            delwin (ptr_win->win_status);
        if (ptr_win->win_input)
            delwin (ptr_win->win_input);
        /* TODO: free input buffer, lines, messages, completion */
    }
    
    /* end of curses output */
    refresh ();
    endwin ();
}

/*
 * gui_add_message: add a message to a window
 */

void
gui_add_message (t_gui_window *window, int type, int color, char *message)
{
    char *pos;
    int length;
    
    /* create new line if previous was ending by '\n' (or if 1st line) */
    if (window->line_complete)
    {
        window->line_complete = 0;
        if (!gui_new_line (window))
            return;
    }
    if (!gui_new_message (window))
        return;
    
    window->last_line->last_message->type = type;
    window->last_line->last_message->color = color;
    pos = strchr (message, '\n');
    if (pos)
    {
        pos[0] = '\0';
        window->line_complete = 1;
    }
    window->last_line->last_message->message = strdup (message);
    length = strlen (message);
    window->last_line->length += length;
    if (type == MSG_TYPE_MSG)
        window->last_line->line_with_message = 1;
    if ((type == MSG_TYPE_TIME) || (type == MSG_TYPE_NICK))
        window->last_line->length_align += length;
    if (pos)
    {
        pos[0] = '\n';
        if ((window == gui_current_window) && (window->sub_lines == 0))
        {
            if ((window->win_chat_cursor_y
                + gui_get_line_num_splits (window, window->last_line)) >
                (window->win_chat_height - 1))
                gui_draw_window_chat (window);
            else
                gui_display_line (window, window->last_line, 1);
        }
        if ((window != gui_current_window) || (window->sub_lines > 0))
        {
            window->unread_data = 1 + window->last_line->line_with_message;
            gui_redraw_window_status (gui_current_window);
        }
    }
}

/*
 * gui_printf_color_type: display a message in a window
 */

void
gui_printf_color_type (t_gui_window *window, int type, int color, char *message, ...)
{
    static char buffer[8192];
    char timestamp[16];
    char *pos;
    va_list argptr;
    static time_t seconds;
    struct tm *date_tmp;

    if (gui_ready)
    {
        if (color == -1)
            color = COLOR_WIN_CHAT;
    
        if (window == NULL)
        {
            if (SERVER(gui_current_window))
                window = SERVER(gui_current_window)->window;
            else
                window = gui_current_window;
        }
    
        if (window == NULL)
        {
            log_printf ("gui_printf without window! this is a bug, please send to developers - thanks\n");
            return;
        }
    }
    
    va_start (argptr, message);
    vsnprintf (buffer, sizeof (buffer) - 1, message, argptr);
    va_end (argptr);
    
    if (gui_ready)
    {
        seconds = time (NULL);
        date_tmp = localtime (&seconds);
        
        pos = buffer - 1;
        while (pos)
        {
            /* TODO: read timestamp format from config! */
            if ((!window->last_line) || (window->line_complete))
            {
                gui_add_message (window, MSG_TYPE_TIME, COLOR_WIN_CHAT_DARK, "[");
                sprintf (timestamp, "%02d", date_tmp->tm_hour);
                gui_add_message (window, MSG_TYPE_TIME, COLOR_WIN_CHAT_TIME, timestamp);
                gui_add_message (window, MSG_TYPE_TIME, COLOR_WIN_CHAT_TIME_SEP, ":");
                sprintf (timestamp, "%02d", date_tmp->tm_min);
                gui_add_message (window, MSG_TYPE_TIME, COLOR_WIN_CHAT_TIME, timestamp);
                gui_add_message (window, MSG_TYPE_TIME, COLOR_WIN_CHAT_TIME_SEP, ":");
                sprintf (timestamp, "%02d", date_tmp->tm_sec);
                gui_add_message (window, MSG_TYPE_TIME, COLOR_WIN_CHAT_TIME, timestamp);
                gui_add_message (window, MSG_TYPE_TIME, COLOR_WIN_CHAT_DARK, "] ");
            }
            gui_add_message (window, type, color, pos + 1);
            pos = strchr (pos + 1, '\n');
            if (pos && !pos[1])
                pos = NULL;
        }
        
        wrefresh (window->win_chat);
        refresh ();
    }
    else
        printf ("%s", buffer);
}
