/*
 * Copyright (c) 2004 by FlashCode <flashcode@flashtux.org>
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
#include "../../common/hotlist.h"
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
 * gui_buffer_has_nicklist: returns 1 if buffer has nicklist
 */

int
gui_buffer_has_nicklist (t_gui_buffer *buffer)
{
    return (((CHANNEL(buffer)) && (CHANNEL(buffer)->type == CHAT_CHANNEL)) ? 1 : 0);
}


/*
 * gui_calculate_pos_size: calculate position and size for a buffer & subwindows
 */

void
gui_calculate_pos_size (t_gui_window *window)
{
    int max_length, lines;
    int num_nicks, num_op, num_halfop, num_voice, num_normal;
    
    if (!gui_ok)
        return;
    
    /* init chat & nicklist settings */
    if (cfg_look_nicklist && BUFFER_IS_CHANNEL(window->buffer))
    {
        max_length = nick_get_max_length (CHANNEL(window->buffer));
        
        switch (cfg_look_nicklist_position)
        {
            case CFG_LOOK_NICKLIST_LEFT:
                window->win_chat_x = window->win_x + max_length + 2;
                window->win_chat_y = window->win_y + 1;
                window->win_chat_width = window->win_width - max_length - 2;
                window->win_nick_x = window->win_x + 0;
                window->win_nick_y = window->win_y + 1;
                window->win_nick_width = max_length + 2;
                if (cfg_look_infobar)
                {
                    window->win_chat_height = window->win_height - 4;
                    window->win_nick_height = window->win_height - 4;
                }
                else
                {
                    window->win_chat_height = window->win_height - 3;
                    window->win_nick_height = window->win_height - 3;
                }
                break;
            case CFG_LOOK_NICKLIST_RIGHT:
                window->win_chat_x = window->win_x;
                window->win_chat_y = window->win_y + 1;
                window->win_chat_width = window->win_width - max_length - 2;
                window->win_nick_x = window->win_x + window->win_width - max_length - 2;
                window->win_nick_y = window->win_y + 1;
                window->win_nick_width = max_length + 2;
                if (cfg_look_infobar)
                {
                    window->win_chat_height = window->win_height - 4;
                    window->win_nick_height = window->win_height - 4;
                }
                else
                {
                    window->win_chat_height = window->win_height - 3;
                    window->win_nick_height = window->win_height - 3;
                }
                break;
            case CFG_LOOK_NICKLIST_TOP:
                nick_count (CHANNEL(window->buffer), &num_nicks, &num_op,
                            &num_halfop, &num_voice, &num_normal);
                if (((max_length + 2) * num_nicks) % window->win_width == 0)
                    lines = ((max_length + 2) * num_nicks) / window->win_width;
                else
                    lines = (((max_length + 2) * num_nicks) / window->win_width) + 1;
                window->win_chat_x = window->win_x;
                window->win_chat_y = window->win_y + 1 + (lines + 1);
                window->win_chat_width = window->win_width;
                if (cfg_look_infobar)
                    window->win_chat_height = window->win_height - 3 - (lines + 1) - 1;
                else
                    window->win_chat_height = window->win_height - 3 - (lines + 1);
                window->win_nick_x = window->win_x;
                window->win_nick_y = window->win_y + 1;
                window->win_nick_width = window->win_width;
                window->win_nick_height = lines + 1;
                break;
            case CFG_LOOK_NICKLIST_BOTTOM:
                nick_count (CHANNEL(window->buffer), &num_nicks, &num_op,
                            &num_halfop, &num_voice, &num_normal);
                if (((max_length + 2) * num_nicks) % window->win_width == 0)
                    lines = ((max_length + 2) * num_nicks) / window->win_width;
                else
                    lines = (((max_length + 2) * num_nicks) / window->win_width) + 1;
                window->win_chat_x = window->win_x;
                window->win_chat_y = window->win_y + 1;
                window->win_chat_width = window->win_width;
                if (cfg_look_infobar)
                    window->win_chat_height = window->win_height - 3 - (lines + 1) - 1;
                else
                    window->win_chat_height = window->win_height - 3 - (lines + 1);
                window->win_nick_x = window->win_x;
                if (cfg_look_infobar)
                    window->win_nick_y = window->win_y + window->win_height - 2 - (lines + 1) - 1;
                else
                    window->win_nick_y = window->win_y + window->win_height - 2 - (lines + 1);
                window->win_nick_width = window->win_width;
                window->win_nick_height = lines + 1;
                break;
        }
        
        window->win_chat_cursor_x = window->win_x;
        window->win_chat_cursor_y = window->win_y;
    }
    else
    {
        window->win_chat_x = window->win_x;
        window->win_chat_y = window->win_y + 1;
        window->win_chat_width = window->win_width;
        if (cfg_look_infobar)
            window->win_chat_height = window->win_height - 4;
        else
            window->win_chat_height = window->win_height - 3;
        window->win_chat_cursor_x = window->win_x;
        window->win_chat_cursor_y = window->win_y;
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
    if (!gui_ok)
        return;
    
    werase (window);
    wmove (window, 0, 0);
}

/*
 * gui_draw_buffer_title: draw title window for a buffer
 */

void
gui_draw_buffer_title (t_gui_buffer *buffer, int erase)
{
    t_gui_window *ptr_win;
    char format[32];
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            if (erase)
                gui_curses_window_clear (ptr_win->win_title);
            
            if (has_colors ())
            {
                gui_window_set_color (ptr_win->win_title, COLOR_WIN_TITLE);
                wborder (ptr_win->win_title, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
                wrefresh (ptr_win->win_title);
                refresh ();
            }
            if (CHANNEL(buffer))
            {
                snprintf (format, 32, "%%-%ds", ptr_win->win_width);
                if (CHANNEL(buffer)->topic)
                    mvwprintw (ptr_win->win_title, 0, 0, format,
                               CHANNEL(buffer)->topic);
            }
            else
            {
                if (!buffer->dcc)
                {
                    /* TODO: change this copyright as title? */
                    mvwprintw (ptr_win->win_title, 0, 0,
                               "%s", PACKAGE_STRING " - " WEECHAT_WEBSITE);
                    mvwprintw (ptr_win->win_title, 0,
                               ptr_win->win_width - strlen (WEECHAT_COPYRIGHT),
                               "%s", WEECHAT_COPYRIGHT);
                }
            }
            wrefresh (ptr_win->win_title);
            refresh ();
        }
    }
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
    char saved_char, format_align[32], format_empty[32];
    
    snprintf (format_align, 32, "%%-%ds", line->length_align);
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
                {
                    snprintf (format_empty, 32, "%%-%ds",
                              window->win_chat_width - window->win_chat_cursor_x);
                    wprintw (window->win_chat, format_empty, " ");
                }
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
    char saved_char, format_align[32], format_empty[32];
    
    snprintf (format_align, 32, "%%-%ds", line->length_align);
    ptr_message = line->messages;
    offset = 0;
    while (ptr_message)
    {
        /* cursor is below end line of chat window */
        if (window->win_chat_cursor_y > window->win_chat_height - 1)
        {
            /*if (!stop_at_end)
                wscrl (buffer->window->win_chat, +1);*/
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
            (!(ptr_message->type & MSG_TYPE_TIME)) &&
            (!(ptr_message->type & MSG_TYPE_NICK)) &&
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
                {
                    snprintf (format_empty, 32, "%%-%ds",
                              window->win_chat_width - window->win_chat_cursor_x);
                    wprintw (window->win_chat, format_empty, " ");
                }
                window->win_chat_cursor_y++;
            }
            window->win_chat_cursor_x = 0;
        }
    }
    return 1;
}

/*
 * gui_draw_buffer_chat: draw chat window for a buffer
 */

void
gui_draw_buffer_chat (t_gui_buffer *buffer, int erase)
{
    t_gui_window *ptr_win;
    t_gui_line *ptr_line;
    t_dcc *dcc_first, *dcc_selected, *ptr_dcc;
    char format_empty[32];
    int i, j, lines_used, num_bars;
    char *unit_name[] = { N_("bytes"), N_("Kb"), N_("Mb"), N_("Gb") };
    char *unit_format[] = { "%.0Lf", "%.1Lf", "%.02Lf", "%.02Lf" };
    long unit_divide[] = { 1, 1024, 1024*1024, 1024*1024,1024 };
    int num_unit;
    char format[32];
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            if (erase)
            {
                gui_window_set_color (ptr_win->win_chat, COLOR_WIN_CHAT);
                
                snprintf (format_empty, 32, "%%-%ds", ptr_win->win_chat_width);
                for (i = 0; i < ptr_win->win_chat_height; i++)
                {
                    mvwprintw (ptr_win->win_chat, i, 0, format_empty, " ");
                }
            }
            
            gui_window_set_color (ptr_win->win_chat, COLOR_WIN_CHAT);
            
            if (buffer->dcc)
            {
                i = 0;
                dcc_first = (ptr_win->dcc_first) ? (t_dcc *) ptr_win->dcc_first : dcc_list;
                dcc_selected = (ptr_win->dcc_selected) ? (t_dcc *) ptr_win->dcc_selected : dcc_list;
                for (ptr_dcc = dcc_first; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
                {
                    if (i >= ptr_win->win_chat_height - 1)
                        break;
                    if ((ptr_dcc->type == DCC_FILE_RECV)
                        || (ptr_dcc->type == DCC_FILE_SEND))
                    {
                        gui_window_set_color (ptr_win->win_chat,
                                              (ptr_dcc == dcc_selected) ?
                                                  COLOR_DCC_SELECTED : COLOR_WIN_CHAT);
                        mvwprintw (ptr_win->win_chat, i, 0, "%s %-16s %s",
                                   (ptr_dcc == dcc_selected) ? ">>" : "  ",
                                   ptr_dcc->nick, ptr_dcc->filename);
                        if (ptr_dcc->filename_suffix > 0)
                            wprintw (ptr_win->win_chat, " (.%d)",
                                     ptr_dcc->filename_suffix);
                        gui_window_set_color (ptr_win->win_chat,
                                              (ptr_dcc == dcc_selected) ?
                                                  COLOR_DCC_SELECTED : COLOR_WIN_CHAT);
                        mvwprintw (ptr_win->win_chat, i + 1, 0, "%s %s ",
                                   (ptr_dcc == dcc_selected) ? ">>" : "  ",
                                   (ptr_dcc->type == DCC_FILE_RECV) ?
                                   "--->" : "<---");
                        gui_window_set_color (ptr_win->win_chat,
                                              COLOR_DCC_WAITING + ptr_dcc->status);
                        wprintw (ptr_win->win_chat, "%-10s",
                                 dcc_status_string[ptr_dcc->status]);
                        gui_window_set_color (ptr_win->win_chat,
                                              (ptr_dcc == dcc_selected) ?
                                                  COLOR_DCC_SELECTED : COLOR_WIN_CHAT);
                        wprintw (ptr_win->win_chat, "  [",
                                 dcc_status_string[ptr_dcc->status]);
                        if (ptr_dcc->size == 0)
                            num_bars = 10;
                        else
                            num_bars = (int)((((long double)(ptr_dcc->pos)/(long double)(ptr_dcc->size))*100) / 10);
                        for (j = 0; j < num_bars - 1; j++)
                            wprintw (ptr_win->win_chat, "=");
                        if (num_bars > 0)
                            wprintw (ptr_win->win_chat, ">");
                        for (j = 0; j < 10 - num_bars; j++)
                            wprintw (ptr_win->win_chat, " ");
                        if (ptr_dcc->size < 1024*10)
                            num_unit = 0;
                        else if (ptr_dcc->size < 1024*1024)
                            num_unit = 1;
                        else if (ptr_dcc->size < 1024*1024*1024)
                            num_unit = 2;
                        else
                            num_unit = 3;
                        wprintw (ptr_win->win_chat, "] %3lu%%   ",
                                 (unsigned long)(((long double)(ptr_dcc->pos)/(long double)(ptr_dcc->size))*100),
                                 dcc_status_string[ptr_dcc->status]);
                        sprintf (format, "%s %%s / %s %%s",
                                 unit_format[num_unit],
                                 unit_format[num_unit]);
                        wprintw (ptr_win->win_chat, format,
                                 ((long double) ptr_dcc->pos) / ((long double)(unit_divide[num_unit])),
                                 unit_name[num_unit],
                                 ((long double) ptr_dcc->size) / ((long double)(unit_divide[num_unit])),
                                 unit_name[num_unit]);
                        ptr_win->dcc_last_displayed = ptr_dcc;
                        i += 2;
                    }
                }
            }
            else
            {
                ptr_line = buffer->last_line;
                lines_used = 0;
                while (ptr_line
                    && (lines_used < (ptr_win->win_chat_height + ptr_win->sub_lines)))
                {
                    lines_used += gui_get_line_num_splits (ptr_win, ptr_line);
                    ptr_line = ptr_line->prev_line;
                }
                ptr_win->win_chat_cursor_x = 0;
                ptr_win->win_chat_cursor_y = 0;
                if (lines_used > (ptr_win->win_chat_height + ptr_win->sub_lines))
                {
                    /* screen will be full (we'll display only end of 1st line) */
                    ptr_line = (ptr_line) ? ptr_line->next_line : buffer->lines;
                    gui_display_end_of_line (ptr_win, ptr_line,
                                             gui_get_line_num_splits (ptr_win, ptr_line) -
                                             (lines_used - (ptr_win->win_chat_height + ptr_win->sub_lines)));
                    ptr_line = ptr_line->next_line;
                    ptr_win->first_line_displayed = 0;
                }
                else
                {
                    /* all lines are displayed */
                    if (!ptr_line)
                    {
                        ptr_win->first_line_displayed = 1;
                        ptr_line = buffer->lines;
                    }
                    else
                    {
                        ptr_win->first_line_displayed = 0;
                        ptr_line = ptr_line->next_line;
                    }
                }
                while (ptr_line)
                {
                    if (!gui_display_line (ptr_win, ptr_line, 1))
                        break;
                    
                    ptr_line = ptr_line->next_line;
                }
                /*if (ptr_win->win_chat_cursor_y <= ptr_win->win_chat_height - 1)
                    buffer->sub_lines = 0;*/
            }
            wrefresh (ptr_win->win_chat);
            refresh ();
        }
    }
}

/*
 * gui_draw_buffer_nick: draw nick window for a buffer
 */

void
gui_draw_buffer_nick (t_gui_buffer *buffer, int erase)
{
    t_gui_window *ptr_win;
    int i, x, y, column, max_length;
    char format[32], format_empty[32];
    t_irc_nick *ptr_nick;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            if (erase)
            {
                gui_window_set_color (ptr_win->win_nick, COLOR_WIN_NICK);
                
                snprintf (format_empty, 32, "%%-%ds", ptr_win->win_nick_width);
                for (i = 0; i < ptr_win->win_nick_height; i++)
                {
                    mvwprintw (ptr_win->win_nick, i, 0, format_empty, " ");
                }
            }
            
            if (CHANNEL(buffer) && CHANNEL(buffer)->nicks)
            {
                max_length = nick_get_max_length (CHANNEL(buffer));
                if ((buffer == gui_current_window->buffer) &&
                    ((max_length + 2) != ptr_win->win_nick_width))
                {
                    gui_calculate_pos_size (ptr_win);
                    delwin (ptr_win->win_chat);
                    delwin (ptr_win->win_nick);
                    ptr_win->win_chat = newwin (ptr_win->win_chat_height,
                                                ptr_win->win_chat_width,
                                                ptr_win->win_chat_y,
                                                ptr_win->win_chat_x);
                    ptr_win->win_nick = newwin (ptr_win->win_nick_height,
                                                ptr_win->win_nick_width,
                                                ptr_win->win_nick_y,
                                                ptr_win->win_nick_x);
                    gui_draw_buffer_chat (buffer, 1);
                    
                    gui_window_set_color (ptr_win->win_nick, COLOR_WIN_NICK);
                    
                    snprintf (format_empty, 32, "%%-%ds", ptr_win->win_nick_width);
                    for (i = 0; i < ptr_win->win_nick_height; i++)
                    {
                        mvwprintw (ptr_win->win_nick, i, 0, format_empty, " ");
                    }
                }
                snprintf (format, 32, "%%-%ds", max_length);
                
                if (has_colors ())
                {
                    switch (cfg_look_nicklist_position)
                    {
                        case CFG_LOOK_NICKLIST_LEFT:
                            gui_window_set_color (ptr_win->win_nick, COLOR_WIN_NICK_SEP);
                            for (i = 0; i < ptr_win->win_chat_height; i++)
                                mvwprintw (ptr_win->win_nick,
                                           i, ptr_win->win_nick_width - 1, " ");
                            break;
                        case CFG_LOOK_NICKLIST_RIGHT:
                            gui_window_set_color (ptr_win->win_nick, COLOR_WIN_NICK_SEP);
                            for (i = 0; i < ptr_win->win_chat_height; i++)
                                mvwprintw (ptr_win->win_nick,
                                           i, 0, " ");
                            break;
                        case CFG_LOOK_NICKLIST_TOP:
                            gui_window_set_color (ptr_win->win_nick, COLOR_WIN_NICK);
                            for (i = 0; i < ptr_win->win_chat_width; i += 2)
                                mvwprintw (ptr_win->win_nick,
                                           ptr_win->win_nick_height - 1, i, "-");
                            break;
                        case CFG_LOOK_NICKLIST_BOTTOM:
                            gui_window_set_color (ptr_win->win_nick, COLOR_WIN_NICK);
                            for (i = 0; i < ptr_win->win_chat_width; i += 2)
                                mvwprintw (ptr_win->win_nick,
                                           0, i, "-");
                            break;
                    }
                }
                
                gui_window_set_color (ptr_win->win_nick, COLOR_WIN_NICK);
                x = 0;
                y = (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_BOTTOM) ? 1 : 0;
                column = 0;
                for (ptr_nick = CHANNEL(buffer)->nicks; ptr_nick;
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
                        gui_window_set_color (ptr_win->win_nick, COLOR_WIN_NICK_OP);
                        mvwprintw (ptr_win->win_nick, y, x, "@");
                        x++;
                    }
                    else
                    {
                        if (ptr_nick->is_halfop)
                        {
                            gui_window_set_color (ptr_win->win_nick, COLOR_WIN_NICK_HALFOP);
                            mvwprintw (ptr_win->win_nick, y, x, "%%");
                            x++;
                        }
                        else
                        {
                            if (ptr_nick->has_voice)
                            {
                                gui_window_set_color (ptr_win->win_nick, COLOR_WIN_NICK_VOICE);
                                mvwprintw (ptr_win->win_nick, y, x, "+");
                                x++;
                            }
                            else
                            {
                                gui_window_set_color (ptr_win->win_nick, COLOR_WIN_NICK);
                                mvwprintw (ptr_win->win_nick, y, x, " ");
                                x++;
                            }
                        }
                    }
                    gui_window_set_color (ptr_win->win_nick, COLOR_WIN_NICK);
                    mvwprintw (ptr_win->win_nick, y, x, format, ptr_nick->nick);
                    y++;
                    if ((cfg_look_nicklist_position == CFG_LOOK_NICKLIST_TOP) ||
                        (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_BOTTOM))
                    {
                        if (y - ((cfg_look_nicklist_position == CFG_LOOK_NICKLIST_BOTTOM) ? 1 : 0) >= ptr_win->win_nick_height - 1)
                        {
                            column += max_length + 2;
                            y = (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_TOP) ?
                                0 : 1;
                        }
                    }
                }
            }
            wrefresh (ptr_win->win_nick);
            refresh ();
        }
    }
}

/*
 * gui_draw_buffer_status: draw status window for a buffer
 */

void
gui_draw_buffer_status (t_gui_buffer *buffer, int erase)
{
    t_gui_window *ptr_win;
    t_weechat_hotlist *ptr_hotlist;
    char format_more[32];
    int i, first_mode;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (erase)
            gui_curses_window_clear (ptr_win->win_status);
        
        if (has_colors ())
        {
            gui_window_set_color (ptr_win->win_status, COLOR_WIN_STATUS);
            wborder (ptr_win->win_status, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
            wrefresh (ptr_win->win_status);
        }
        wmove (ptr_win->win_status, 0, 0);
        
        /* display number of buffers */
        gui_window_set_color (ptr_win->win_status,
                              COLOR_WIN_STATUS_DELIMITERS);
        wprintw (ptr_win->win_status, "[");
        gui_window_set_color (ptr_win->win_status,
                              COLOR_WIN_STATUS);
        wprintw (ptr_win->win_status, "%d",
                 (last_gui_buffer) ? last_gui_buffer->number : 0);
        gui_window_set_color (ptr_win->win_status,
                              COLOR_WIN_STATUS_DELIMITERS);
        wprintw (ptr_win->win_status, "] ");
        
        /* display current server */
        if (SERVER(ptr_win->buffer) && SERVER(ptr_win->buffer)->name)
        {
            wprintw (ptr_win->win_status, "[");
            gui_window_set_color (ptr_win->win_status,
                                  COLOR_WIN_STATUS);
            wprintw (ptr_win->win_status, "%s", SERVER(ptr_win->buffer)->name);
            gui_window_set_color (ptr_win->win_status,
                              COLOR_WIN_STATUS_DELIMITERS);
            wprintw (ptr_win->win_status, "] ");
        }
        if (SERVER(ptr_win->buffer) && !CHANNEL(ptr_win->buffer))
        {
            gui_window_set_color (ptr_win->win_status,
                                  COLOR_WIN_STATUS);
            wprintw (ptr_win->win_status, "%d",
                     ptr_win->buffer->number);
            gui_window_set_color (ptr_win->win_status,
                                  COLOR_WIN_STATUS_DELIMITERS);
            wprintw (ptr_win->win_status, ":");
            gui_window_set_color (ptr_win->win_status,
                                  COLOR_WIN_STATUS);
            if (SERVER(ptr_win->buffer)->is_connected)
                wprintw (ptr_win->win_status, "[%s] ",
                         SERVER(ptr_win->buffer)->name);
            else
                wprintw (ptr_win->win_status, "(%s) ",
                         SERVER(ptr_win->buffer)->name);
        }
        if (SERVER(ptr_win->buffer) && CHANNEL(ptr_win->buffer))
        {
            gui_window_set_color (ptr_win->win_status,
                                  COLOR_WIN_STATUS);
            wprintw (ptr_win->win_status, "%d",
                     ptr_win->buffer->number);
            gui_window_set_color (ptr_win->win_status,
                                  COLOR_WIN_STATUS_DELIMITERS);
            wprintw (ptr_win->win_status, ":");
            gui_window_set_color (ptr_win->win_status,
                                  COLOR_WIN_STATUS);
            wprintw (ptr_win->win_status, "%s",
                     CHANNEL(ptr_win->buffer)->name);
            if (ptr_win->buffer == CHANNEL(ptr_win->buffer)->buffer)
            {
                /* display channel modes */
                wprintw (ptr_win->win_status, "(");
                i = 0;
                first_mode = 1;
                while (CHANNEL(ptr_win->buffer)->modes[i])
                {
                    if (CHANNEL(ptr_win->buffer)->modes[i] != ' ')
                    {
                        if (first_mode)
                        {
                            wprintw (ptr_win->win_status, "+");
                            first_mode = 0;
                        }
                        wprintw (ptr_win->win_status, "%c",
                                 CHANNEL(ptr_win->buffer)->modes[i]);
                    }
                    i++;
                }
                if (CHANNEL(ptr_win->buffer)->modes[CHANNEL_MODE_KEY] != ' ')
                    wprintw (ptr_win->win_status, ",%s",
                             CHANNEL(ptr_win->buffer)->key);
                if (CHANNEL(ptr_win->buffer)->modes[CHANNEL_MODE_LIMIT] != ' ')
                    wprintw (ptr_win->win_status, ",%d",
                             CHANNEL(ptr_win->buffer)->limit);
                wprintw (ptr_win->win_status, ")");
            }
            wprintw (ptr_win->win_status, " ");
        }
        if (!SERVER(ptr_win->buffer))
        {
            gui_window_set_color (ptr_win->win_status, COLOR_WIN_STATUS);
            if (ptr_win->buffer->dcc)
                wprintw (ptr_win->win_status, "%d:<DCC> ",
                         ptr_win->buffer->number);
            else
                wprintw (ptr_win->win_status, _("%d:[not connected] "),
                         ptr_win->buffer->number);
        }
        
        /* display list of other active windows (if any) with numbers */
        if (hotlist)
        {
            gui_window_set_color (ptr_win->win_status, COLOR_WIN_STATUS);
            wprintw (ptr_win->win_status, _("[Act: "));
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                switch (ptr_hotlist->priority)
                {
                    case 0:
                        gui_window_set_color (ptr_win->win_status,
                                              COLOR_WIN_STATUS_DATA_OTHER);
                        break;
                    case 1:
                        gui_window_set_color (ptr_win->win_status,
                                              COLOR_WIN_STATUS_DATA_MSG);
                        break;
                    case 2:
                        gui_window_set_color (ptr_win->win_status,
                                              COLOR_WIN_STATUS_DATA_HIGHLIGHT);
                        break;
                }
                if (ptr_hotlist->buffer->dcc)
                    wprintw (ptr_win->win_status, "%d/DCC",
                             ptr_hotlist->buffer->number);
                else
                    wprintw (ptr_win->win_status, "%d",
                             ptr_hotlist->buffer->number);
                gui_window_set_color (ptr_win->win_status,
                                      COLOR_WIN_STATUS);
                if (ptr_hotlist->next_hotlist)
                    wprintw (ptr_win->win_status, ",");
            }
            gui_window_set_color (ptr_win->win_status,
                                  COLOR_WIN_STATUS);
            wprintw (ptr_win->win_status, "]");
        }
        
        /* display "-MORE-" if last line is not displayed */
        gui_window_set_color (ptr_win->win_status, COLOR_WIN_STATUS_MORE);
        if (ptr_win->sub_lines > 0)
            mvwprintw (ptr_win->win_status, 0, ptr_win->win_width - 7,
                       _("-MORE-"));
        else
        {
            snprintf (format_more, 32, "%%-%ds", strlen (_("-MORE-")));
            mvwprintw (ptr_win->win_status, 0, ptr_win->win_width - 7,
                       format_more, " ");
        }
        
        wrefresh (ptr_win->win_status);
        refresh ();
    }
}

/*
 * gui_draw_buffer_infobar: draw infobar window for a buffer
 */

void
gui_draw_buffer_infobar (t_gui_buffer *buffer, int erase)
{
    t_gui_window *ptr_win;
    time_t time_seconds;
    struct tm *local_time;
    char text[1024 + 1];
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            if (erase)
                gui_curses_window_clear (ptr_win->win_infobar);
    
            if (has_colors ())
            {
                gui_window_set_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR);
                wborder (ptr_win->win_infobar, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
                wrefresh (ptr_win->win_infobar);
            }
            wmove (ptr_win->win_infobar, 0, 0);
            
            time_seconds = time (NULL);
            local_time = localtime (&time_seconds);
            if (local_time)
            {
                strftime (text, 1024, cfg_look_infobar_timestamp, local_time);
                gui_window_set_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR);
                wprintw (ptr_win->win_infobar, "%s", text);
            }
            if (gui_infobar)
            {
                gui_window_set_color (ptr_win->win_infobar, gui_infobar->color);
                wprintw (ptr_win->win_infobar, " | %s", gui_infobar->text);
            }
            
            wrefresh (ptr_win->win_infobar);
            refresh ();
        }
    }
}

/*
 * gui_get_input_width: return input width (max # chars displayed)
 */

int
gui_get_input_width (t_gui_window *window)
{
    if (CHANNEL(window->buffer))
        return (window->win_width - strlen (CHANNEL(window->buffer)->name) -
                strlen (SERVER(window->buffer)->nick) - 3);
    else
    {
        if (SERVER(window->buffer) && (SERVER(window->buffer)->is_connected))
            return (window->win_width - strlen (SERVER(window->buffer)->nick) - 2);
        else
            return (window->win_width - strlen (cfg_look_no_nickname) - 2);
    }
}

/*
 * gui_draw_buffer_input: draw input window for a buffer
 */

void
gui_draw_buffer_input (t_gui_buffer *buffer, int erase)
{
    t_gui_window *ptr_win;
    char format[32];
    char *ptr_nickname;
    int input_width;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            if (erase)
                gui_curses_window_clear (ptr_win->win_input);
    
            if (has_colors ())
            {
                gui_window_set_color (ptr_win->win_input, COLOR_WIN_INPUT);
                wborder (ptr_win->win_input, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
                wrefresh (ptr_win->win_input);
            }
            
            if (buffer->input_buffer_size == 0)
                buffer->input_buffer[0] = '\0';
            
            input_width = gui_get_input_width (ptr_win);
            
            if (buffer->input_buffer_pos - buffer->input_buffer_1st_display + 1 >
                input_width)
                buffer->input_buffer_1st_display = buffer->input_buffer_pos -
                    input_width + 1;
            else
            {
                if (buffer->input_buffer_pos < buffer->input_buffer_1st_display)
                    buffer->input_buffer_1st_display = buffer->input_buffer_pos;
                else
                {
                    if ((buffer->input_buffer_1st_display > 0) &&
                        (buffer->input_buffer_pos -
                        buffer->input_buffer_1st_display + 1) < input_width)
                    {
                        buffer->input_buffer_1st_display =
                            buffer->input_buffer_pos - input_width + 1;
                        if (buffer->input_buffer_1st_display < 0)
                            buffer->input_buffer_1st_display = 0;
                    }
                }
            }
            if (CHANNEL(buffer))
            {
                snprintf (format, 32, "%%s %%s> %%-%ds", input_width);
                if (ptr_win == gui_current_window)
                    mvwprintw (ptr_win->win_input, 0, 0, format,
                               CHANNEL(buffer)->name,
                               SERVER(buffer)->nick,
                               buffer->input_buffer + buffer->input_buffer_1st_display);
                else
                    mvwprintw (ptr_win->win_input, 0, 0, format,
                               CHANNEL(buffer)->name,
                               SERVER(buffer)->nick,
                               "");
                wclrtoeol (ptr_win->win_input);
                if (ptr_win == gui_current_window)
                    move (ptr_win->win_y + ptr_win->win_height - 1,
                          ptr_win->win_x + strlen (CHANNEL(buffer)->name) +
                          strlen (SERVER(buffer)->nick) + 3 +
                          (buffer->input_buffer_pos - buffer->input_buffer_1st_display));
            }
            else
            {
                if (buffer->dcc)
                {
                    snprintf (format, 32, "%%-%ds", input_width);
                    mvwprintw (ptr_win->win_input, 0, 0, format, "");
                    wclrtoeol (ptr_win->win_input);
                    if (ptr_win == gui_current_window)
                        move (ptr_win->win_y + ptr_win->win_height - 1,
                              ptr_win->win_x);
                }
                else
                {
                    snprintf (format, 32, "%%s> %%-%ds", input_width);
                    if (SERVER(buffer) && (SERVER(buffer)->is_connected))
                        ptr_nickname = SERVER(buffer)->nick;
                    else
                        ptr_nickname = cfg_look_no_nickname;
                    if (ptr_win == gui_current_window)
                        mvwprintw (ptr_win->win_input, 0, 0, format,
                                   ptr_nickname,
                                   buffer->input_buffer + buffer->input_buffer_1st_display);
                    else
                        mvwprintw (ptr_win->win_input, 0, 0, format,
                                   ptr_nickname,
                                   "");
                    wclrtoeol (ptr_win->win_input);
                    if (ptr_win == gui_current_window)
                        move (ptr_win->win_y + ptr_win->win_height - 1,
                              ptr_win->win_x + strlen (ptr_nickname) + 2 +
                              (buffer->input_buffer_pos - buffer->input_buffer_1st_display));
                }
            }
            
            wrefresh (ptr_win->win_input);
            refresh ();
        }
    }
}

/*
 * gui_redraw_buffer: redraw a buffer
 */

void
gui_redraw_buffer (t_gui_buffer *buffer)
{
    t_gui_window *ptr_win;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            gui_draw_buffer_title (buffer, 1);
            gui_draw_buffer_chat (buffer, 1);
            if (ptr_win->win_nick)
                gui_draw_buffer_nick (buffer, 1);
            gui_draw_buffer_status (buffer, 1);
            if (cfg_look_infobar)
                gui_draw_buffer_infobar (buffer, 1);
            gui_draw_buffer_input (buffer, 1);
        }
    }
}

/*
 * gui_switch_to_buffer: switch to another buffer
 */

void
gui_switch_to_buffer (t_gui_window *window, t_gui_buffer *buffer)
{
    if (!gui_ok)
        return;
    
    if (gui_current_window->buffer->num_displayed > 0)
        gui_current_window->buffer->num_displayed--;
    
    window->buffer = buffer;
    gui_calculate_pos_size (window);
    
    /* destroy Curses windows */
    if (window->win_title)
    {
        delwin (window->win_title);
        window->win_title = NULL;
    }
    if (window->win_nick)
    {
        delwin (window->win_nick);
        window->win_nick = NULL;
    }
    if (window->win_status)
    {
        delwin (window->win_status);
        window->win_status = NULL;
    }
    if (window->win_infobar)
    {
        delwin (window->win_infobar);
        window->win_infobar = NULL;
    }
    if (window->win_input)
    {
        delwin (window->win_input);
        window->win_input = NULL;
    }
        
    /* create Curses windows */
    window->win_title = newwin (1,
                                window->win_width,
                                window->win_y,
                                window->win_x);
    window->win_input = newwin (1,
                                window->win_width,
                                window->win_y + window->win_height - 1,
                                window->win_x);
    if (BUFFER_IS_CHANNEL(buffer))
    {
        if (window->win_chat)
            delwin (window->win_chat);
        window->win_chat = newwin (window->win_chat_height,
                                   window->win_chat_width,
                                   window->win_chat_y,
                                   window->win_chat_x);
        if (cfg_look_nicklist)
            window->win_nick = newwin (window->win_nick_height,
                                       window->win_nick_width,
                                       window->win_nick_y,
                                       window->win_nick_x);
        else
            window->win_nick = NULL;
    }
    if (!(BUFFER_IS_CHANNEL(buffer)))
    {
        if (window->win_chat)
            delwin (window->win_chat);
        window->win_chat = newwin (window->win_chat_height,
                                   window->win_chat_width,
                                   window->win_chat_y,
                                   window->win_chat_x);
    }
    
    /* create status/infobar windows */
    if (cfg_look_infobar)
    {
        window->win_infobar = newwin (1, window->win_width, window->win_y + window->win_height - 2, window->win_x);
        window->win_status = newwin (1, window->win_width, window->win_y + window->win_height - 3, window->win_x);
    }
    else
        window->win_status = newwin (1, window->win_width, window->win_y + window->win_height - 2, window->win_x);
    
    window->sub_lines = 0;
    
    buffer->num_displayed++;
    
    hotlist_remove_buffer (buffer);
}

/*
 * gui_get_dcc_buffer: get pointer to DCC buffer (DCC buffer created if not existing)
 */

t_gui_buffer *
gui_get_dcc_buffer ()
{
    t_gui_buffer *ptr_buffer;
    
    /* check if dcc buffer exists */
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        if (BUFFER_IS_DCC (ptr_buffer))
            break;
    }
    if (ptr_buffer)
        return ptr_buffer;
    else
        return gui_buffer_new (gui_current_window, NULL, NULL, 1, 0);
}

/*
 * gui_switch_to_dcc_buffer: switch to dcc buffer (create it if it does not exist)
 */

void
gui_switch_to_dcc_buffer ()
{
    t_gui_buffer *ptr_buffer;
    
    if (!gui_ok)
        return;
    
    /* check if dcc buffer exists */
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        if (BUFFER_IS_DCC (ptr_buffer))
            break;
    }
    if (ptr_buffer)
    {
        gui_switch_to_buffer (gui_current_window, ptr_buffer);
        gui_redraw_buffer (ptr_buffer);
    }
    else
        gui_buffer_new (gui_current_window, NULL, NULL, 1, 1);
}

/*
 * gui_switch_to_previous_buffer: switch to previous buffer
 */

void
gui_switch_to_previous_buffer (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    /* if only one buffer then return */
    if (gui_buffers == last_gui_buffer)
        return;
    
    if (window->buffer->prev_buffer)
        gui_switch_to_buffer (window, window->buffer->prev_buffer);
    else
        gui_switch_to_buffer (window, last_gui_buffer);
    
    gui_redraw_buffer (window->buffer);
}

/*
 * gui_switch_to_next_buffer: switch to next buffer
 */

void
gui_switch_to_next_buffer (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    /* if only one buffer then return */
    if (gui_buffers == last_gui_buffer)
        return;
    
    if (window->buffer->next_buffer)
        gui_switch_to_buffer (window, window->buffer->next_buffer);
    else
        gui_switch_to_buffer (window, gui_buffers);
    
    gui_redraw_buffer (window->buffer);
}

/*
 * gui_switch_to_next_window: switch to next window
 */

void
gui_switch_to_next_window (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    /* if only one window then return */
    if (gui_windows == last_gui_window)
        return;
    
    gui_current_window = (window->next_window) ? window->next_window : gui_windows;
    gui_switch_to_buffer (gui_current_window, gui_current_window->buffer);
    gui_redraw_buffer (gui_current_window->buffer);
}

/*
 * gui_move_page_up: display previous page on buffer
 */

void
gui_move_page_up (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (!window->first_line_displayed)
    {
        window->sub_lines += window->win_chat_height - 1;
        gui_draw_buffer_chat (window->buffer, 0);
        gui_draw_buffer_status (window->buffer, 0);
    }
}

/*
 * gui_move_page_down: display next page on buffer
 */

void
gui_move_page_down (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (window->sub_lines > 0)
    {
        window->sub_lines -= window->win_chat_height - 1;
        if (window->sub_lines < 0)
            window->sub_lines = 0;
        gui_draw_buffer_chat (window->buffer, 0);
        gui_draw_buffer_status (window->buffer, 0);
    }
}

/*
 * gui_curses_resize_handler: called when term size is modified
 */

void
gui_curses_resize_handler ()
{
    t_gui_window *ptr_win, *old_current_window;
    int old_width, old_height;
    int new_width, new_height;
    
    getmaxyx (stdscr, old_height, old_width);
    
    endwin ();
    refresh ();
    
    getmaxyx (stdscr, new_height, new_width);
    
    old_current_window = gui_current_window;
    
    /* TODO: manage when some windows are outside new term size */
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        ptr_win->dcc_first = NULL;
        ptr_win->dcc_selected = NULL;
        if (ptr_win->win_x + ptr_win->win_width == old_width)
            ptr_win->win_width = new_width - ptr_win->win_x;
        if (ptr_win->win_y + ptr_win->win_height == old_height)
            ptr_win->win_height = new_height - ptr_win->win_y;
        gui_switch_to_buffer (ptr_win, ptr_win->buffer);
    }
    
    gui_current_window = old_current_window;
    if ((new_width > 5) && (new_height > 5))
    {
        gui_ok = 1;
        gui_calculate_pos_size (gui_current_window);
        gui_redraw_buffer (gui_current_window->buffer);
    }
    else
        gui_ok = 0;
}

/*
 * gui_window_init_subviews: init subviews for a WeeChat window
 */

void
gui_window_init_subwindows (t_gui_window *window)
{
    window->win_title = NULL;
    window->win_chat = NULL;
    window->win_nick = NULL;
    window->win_status = NULL;
    window->win_infobar = NULL;
    window->win_input = NULL;
}

/*
 * gui_window_split_horiz: split a window horizontally
 */

void
gui_window_split_horiz (t_gui_window *window)
{
    t_gui_window *new_window;
    int height1, height2;
    
    if (!gui_ok)
        return;
    
    height1 = window->win_height / 2;
    height2 = window->win_height - height1;
    if ((new_window = gui_window_new (window->win_x, window->win_y,
                                      window->win_width, height1)))
    {
        /* reduce old window height (bottom window) */
        window->win_y = new_window->win_y + new_window->win_height;
        window->win_height = height2;
        
        /* assign same buffer for new window (top window) */
        new_window->buffer = window->buffer;
        
        gui_switch_to_buffer (window, window->buffer);
        
        gui_current_window = new_window;
        gui_switch_to_buffer (gui_current_window, gui_current_window->buffer);
        gui_redraw_buffer (gui_current_window->buffer);
    }
}

/*
 * gui_window_split_vertic: split a window vertically
 */

void
gui_window_split_vertic (t_gui_window *window)
{
    t_gui_window *new_window;
    int width1, width2;
    
    if (!gui_ok)
        return;
    
    width1 = window->win_width / 2;
    width2 = window->win_width - width1 - 1;
    if ((new_window = gui_window_new (window->win_x + width1 + 1, window->win_y,
                                      width2, window->win_height)))
    {
        /* reduce old window height (left window) */
        window->win_width = width1;
        
        /* assign same buffer for new window (right window) */
        new_window->buffer = window->buffer;
        
        gui_switch_to_buffer (window, window->buffer);
        
        gui_current_window = new_window;
        gui_switch_to_buffer (gui_current_window, gui_current_window->buffer);
        gui_redraw_buffer (gui_current_window->buffer);
        
        /* create & draw separator if window is not on the left */
        gui_current_window->win_separator = newwin (gui_current_window->win_height,
                                                    1,
                                                    gui_current_window->win_y,
                                                    gui_current_window->win_x - 1);
        gui_window_set_color (gui_current_window->win_separator, COLOR_WIN_TITLE);
        wborder (gui_current_window->win_separator, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        wrefresh (gui_current_window->win_separator);
        refresh ();
    }
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
        init_pair (COLOR_WIN_CHAT_HIGHLIGHT,
            cfg_col_chat_highlight & A_CHARTEXT, cfg_col_chat_bg);
        init_pair (COLOR_WIN_STATUS,
            cfg_col_status & A_CHARTEXT, cfg_col_status_bg);
        init_pair (COLOR_WIN_STATUS_DELIMITERS,
            cfg_col_status_delimiters & A_CHARTEXT, cfg_col_status_bg);
        init_pair (COLOR_WIN_STATUS_DATA_MSG,
            cfg_col_status_data_msg & A_CHARTEXT, cfg_col_status_bg);
        init_pair (COLOR_WIN_STATUS_DATA_HIGHLIGHT,
            cfg_col_status_data_highlight & A_CHARTEXT, cfg_col_status_bg);
        init_pair (COLOR_WIN_STATUS_DATA_OTHER,
            cfg_col_status_data_other & A_CHARTEXT, cfg_col_status_bg);
        init_pair (COLOR_WIN_STATUS_MORE,
            cfg_col_status_more & A_CHARTEXT, cfg_col_status_bg);
        init_pair (COLOR_WIN_INFOBAR,
            cfg_col_infobar & A_CHARTEXT, cfg_col_infobar_bg);
        init_pair (COLOR_WIN_INFOBAR_HIGHLIGHT,
            cfg_col_infobar_highlight & A_CHARTEXT, cfg_col_infobar_bg);
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
        
        init_pair (COLOR_DCC_SELECTED,
            cfg_col_dcc_selected & A_CHARTEXT, cfg_col_chat_bg);
        init_pair (COLOR_DCC_WAITING,
            cfg_col_dcc_waiting & A_CHARTEXT, cfg_col_chat_bg);
        init_pair (COLOR_DCC_CONNECTING,
            cfg_col_dcc_connecting & A_CHARTEXT, cfg_col_chat_bg);
        init_pair (COLOR_DCC_ACTIVE,
            cfg_col_dcc_active & A_CHARTEXT, cfg_col_chat_bg);
        init_pair (COLOR_DCC_DONE,
            cfg_col_dcc_done & A_CHARTEXT, cfg_col_chat_bg);
        init_pair (COLOR_DCC_FAILED,
            cfg_col_dcc_failed & A_CHARTEXT, cfg_col_chat_bg);
        init_pair (COLOR_DCC_ABORTED,
            cfg_col_dcc_aborted & A_CHARTEXT, cfg_col_chat_bg);
            
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
        color_attr[COLOR_WIN_CHAT_HIGHLIGHT - 1] = cfg_col_chat_highlight & A_BOLD;
        color_attr[COLOR_WIN_STATUS - 1] = cfg_col_status & A_BOLD;
        color_attr[COLOR_WIN_STATUS_DELIMITERS - 1] = cfg_col_status_delimiters & A_BOLD;
        color_attr[COLOR_WIN_STATUS_DATA_MSG - 1] = cfg_col_status_data_msg & A_BOLD;
        color_attr[COLOR_WIN_STATUS_DATA_HIGHLIGHT - 1] = cfg_col_status_data_highlight & A_BOLD;
        color_attr[COLOR_WIN_STATUS_DATA_OTHER - 1] = cfg_col_status_data_other & A_BOLD;
        color_attr[COLOR_WIN_STATUS_MORE - 1] = cfg_col_status_more & A_BOLD;
        color_attr[COLOR_WIN_INFOBAR - 1] = cfg_col_infobar & A_BOLD;
        color_attr[COLOR_WIN_INFOBAR_HIGHLIGHT - 1] = cfg_col_infobar_highlight & A_BOLD;
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
        color_attr[COLOR_DCC_SELECTED - 1] = cfg_col_dcc_selected & A_BOLD;
        color_attr[COLOR_DCC_WAITING - 1] = cfg_col_dcc_waiting & A_BOLD;
        color_attr[COLOR_DCC_CONNECTING - 1] = cfg_col_dcc_connecting & A_BOLD;
        color_attr[COLOR_DCC_ACTIVE - 1] = cfg_col_dcc_active & A_BOLD;
        color_attr[COLOR_DCC_DONE - 1] = cfg_col_dcc_done & A_BOLD;
        color_attr[COLOR_DCC_FAILED - 1] = cfg_col_dcc_failed & A_BOLD;
        color_attr[COLOR_DCC_ABORTED - 1] = cfg_col_dcc_aborted & A_BOLD;
    }
}

/*
 * gui_set_window_title: set terminal title
 */

void
gui_set_window_title ()
{
    #ifdef __linux__
    /* set title for term window, not for console */
    if (strcmp (getenv ("TERM"), "linux") != 0)
        printf ("\e]2;" PACKAGE_NAME " " PACKAGE_VERSION "\a\e]1;" PACKAGE_NAME " " PACKAGE_VERSION "\a");
    #endif
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

    gui_infobar = NULL;
    
    gui_ok = ((COLS > 5) && (LINES > 5));
    
    /* create new window/buffer */
    if (gui_window_new (0, 0, COLS, LINES))
    {
        gui_current_window = gui_windows;
        gui_buffer_new (gui_windows, NULL, NULL, 0, 1);
    
        signal (SIGWINCH, gui_curses_resize_handler);
    
        if (cfg_look_set_title)
            gui_set_window_title ();
    
        gui_init_ok = 1;
    }
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
        if (ptr_win->win_infobar)
            delwin (ptr_win->win_infobar);
        if (ptr_win->win_input)
            delwin (ptr_win->win_input);
    }
    
    /* delete all buffers */
    while (gui_buffers)
    {
        gui_buffer_free (gui_buffers, 0);
    }
    
    /* end of curses output */
    refresh ();
    endwin ();
}

/*
 * gui_add_message: add a message to a buffer
 */

void
gui_add_message (t_gui_buffer *buffer, int type, int color, char *message)
{
    char *pos;
    int length;
    
    /* create new line if previous was ending by '\n' (or if 1st line) */
    if (buffer->line_complete)
    {
        buffer->line_complete = 0;
        if (!gui_new_line (buffer))
            return;
    }
    if (!gui_new_message (buffer))
        return;
    
    buffer->last_line->last_message->type = type;
    buffer->last_line->last_message->color = color;
    pos = strchr (message, '\n');
    if (pos)
    {
        pos[0] = '\0';
        buffer->line_complete = 1;
    }
    buffer->last_line->last_message->message = strdup (message);
    length = strlen (message);
    buffer->last_line->length += length;
    if (type & MSG_TYPE_MSG)
        buffer->last_line->line_with_message = 1;
    if (type & MSG_TYPE_HIGHLIGHT)
        buffer->last_line->line_with_highlight = 1;
    if ((type & MSG_TYPE_TIME) || (type & MSG_TYPE_NICK))
        buffer->last_line->length_align += length;
    if (pos)
    {
        pos[0] = '\n';
        if ((buffer->num_displayed > 0) && (gui_current_window->sub_lines == 0))
        {
            /*if ((buffer->window->win_chat_cursor_y
                + gui_get_line_num_splits (buffer, buffer->last_line)) >
                (buffer->window->win_chat_height - 1))*/
                gui_draw_buffer_chat (buffer, 0);
            /*else
                gui_display_line (buffer, buffer->last_line, 1);*/
        }
        if (buffer->num_displayed == 0)
        {
            hotlist_add (buffer->last_line->line_with_message +
                         buffer->last_line->line_with_highlight,
                         buffer);
            gui_draw_buffer_status (gui_current_window->buffer, 1);
        }
    }
}

/*
 * gui_printf_color_type: display a message in a buffer
 */

void
gui_printf_color_type (t_gui_buffer *buffer, int type, int color, char *message, ...)
{
    static char buf[8192];
    char timestamp[16];
    char *pos;
    va_list argptr;
    static time_t seconds;
    struct tm *date_tmp;
    
    if (gui_init_ok)
    {
        if (color == -1)
            color = COLOR_WIN_CHAT;
    
        if (buffer == NULL)
        {
            if (SERVER(gui_current_window->buffer))
                buffer = SERVER(gui_current_window->buffer)->buffer;
            else
                buffer = gui_current_window->buffer;
        }
    
        if (buffer == NULL)
        {
            wee_log_printf ("gui_printf without buffer! this is a bug, please send to developers - thanks\n");
            return;
        }
    }
    
    va_start (argptr, message);
    vsnprintf (buf, sizeof (buf) - 1, message, argptr);
    va_end (argptr);
    
    if (gui_init_ok)
    {
        seconds = time (NULL);
        date_tmp = localtime (&seconds);
        
        pos = buf - 1;
        while (pos)
        {
            /* TODO: read timestamp format from config! */
            if ((!buffer->last_line) || (buffer->line_complete))
            {
                gui_add_message (buffer, MSG_TYPE_TIME, COLOR_WIN_CHAT_DARK, "[");
                snprintf (timestamp, 16, "%02d", date_tmp->tm_hour);
                gui_add_message (buffer, MSG_TYPE_TIME, COLOR_WIN_CHAT_TIME, timestamp);
                gui_add_message (buffer, MSG_TYPE_TIME, COLOR_WIN_CHAT_TIME_SEP, ":");
                snprintf (timestamp, 16, "%02d", date_tmp->tm_min);
                gui_add_message (buffer, MSG_TYPE_TIME, COLOR_WIN_CHAT_TIME, timestamp);
                gui_add_message (buffer, MSG_TYPE_TIME, COLOR_WIN_CHAT_TIME_SEP, ":");
                snprintf (timestamp, 16, "%02d", date_tmp->tm_sec);
                gui_add_message (buffer, MSG_TYPE_TIME, COLOR_WIN_CHAT_TIME, timestamp);
                gui_add_message (buffer, MSG_TYPE_TIME, COLOR_WIN_CHAT_DARK, "] ");
            }
            gui_add_message (buffer, type, color, pos + 1);
            pos = strchr (pos + 1, '\n');
            if (pos && !pos[1])
                pos = NULL;
        }
        
        /*wrefresh (buffer->window->win_chat);
        refresh ();*/
    }
    else
        printf ("%s", buf);
}
