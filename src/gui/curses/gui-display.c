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
 * gui_view_has_nicklist: returns 1 if view has nicklist
 */

int
gui_view_has_nicklist (t_gui_view *view)
{
    return (((CHANNEL(view)) && (CHANNEL(view)->type == CHAT_CHANNEL)) ? 1 : 0);
}


/*
 * gui_calculate_pos_size: calculate position and size for a view & subviews
 */

void
gui_calculate_pos_size (t_gui_view *view)
{
    int max_length, lines;
    int num_nicks, num_op, num_halfop, num_voice, num_normal;
    
    /* global position & size */
    /* TODO: get values from function parameters */
    view->window->win_x = 0;
    view->window->win_y = 0;
    view->window->win_width = COLS;
    view->window->win_height = LINES;
    
    /* init chat & nicklist settings */
    /* TODO: calculate values from function parameters */
    if (cfg_look_nicklist && VIEW_IS_CHANNEL(view))
    {
        max_length = nick_get_max_length (CHANNEL(view));
        
        switch (cfg_look_nicklist_position)
        {
            case CFG_LOOK_NICKLIST_LEFT:
                view->window->win_chat_x = max_length + 2;
                view->window->win_chat_y = 1;
                view->window->win_chat_width = COLS - max_length - 2;
                view->window->win_nick_x = 0;
                view->window->win_nick_y = 1;
                view->window->win_nick_width = max_length + 2;
                if (cfg_look_infobar)
                {
                    view->window->win_chat_height = LINES - 4;    
                    view->window->win_nick_height = LINES - 4;
                }
                else
                {
                    view->window->win_chat_height = LINES - 3;    
                    view->window->win_nick_height = LINES - 3;
                }
                break;
            case CFG_LOOK_NICKLIST_RIGHT:
                view->window->win_chat_x = 0;
                view->window->win_chat_y = 1;
                view->window->win_chat_width = COLS - max_length - 2;
                view->window->win_nick_x = COLS - max_length - 2;
                view->window->win_nick_y = 1;
                view->window->win_nick_width = max_length + 2;
                if (cfg_look_infobar)
                {
                    view->window->win_chat_height = LINES - 4;
                    view->window->win_nick_height = LINES - 4;
                }
                else
                {
                    view->window->win_chat_height = LINES - 3;
                    view->window->win_nick_height = LINES - 3;
                }
                break;
            case CFG_LOOK_NICKLIST_TOP:
                nick_count (CHANNEL(view), &num_nicks, &num_op, &num_halfop,
                            &num_voice, &num_normal);
                if (((max_length + 2) * num_nicks) % COLS == 0)
                    lines = ((max_length + 2) * num_nicks) / COLS;
                else
                    lines = (((max_length + 2) * num_nicks) / COLS) + 1;
                view->window->win_chat_x = 0;
                view->window->win_chat_y = 1 + (lines + 1);
                view->window->win_chat_width = COLS;
                if (cfg_look_infobar)
                    view->window->win_chat_height = LINES - 3 - (lines + 1) - 1;
                else
                    view->window->win_chat_height = LINES - 3 - (lines + 1);
                view->window->win_nick_x = 0;
                view->window->win_nick_y = 1;
                view->window->win_nick_width = COLS;
                view->window->win_nick_height = lines + 1;
                break;
            case CFG_LOOK_NICKLIST_BOTTOM:
                nick_count (CHANNEL(view), &num_nicks, &num_op, &num_halfop,
                            &num_voice, &num_normal);
                if (((max_length + 2) * num_nicks) % COLS == 0)
                    lines = ((max_length + 2) * num_nicks) / COLS;
                else
                    lines = (((max_length + 2) * num_nicks) / COLS) + 1;
                view->window->win_chat_x = 0;
                view->window->win_chat_y = 1;
                view->window->win_chat_width = COLS;
                if (cfg_look_infobar)
                    view->window->win_chat_height = LINES - 3 - (lines + 1) - 1;
                else
                    view->window->win_chat_height = LINES - 3 - (lines + 1);
                view->window->win_nick_x = 0;
                if (cfg_look_infobar)
                    view->window->win_nick_y = LINES - 2 - (lines + 1) - 1;
                else
                    view->window->win_nick_y = LINES - 2 - (lines + 1);
                view->window->win_nick_width = COLS;
                view->window->win_nick_height = lines + 1;
                break;
        }
        
        view->window->win_chat_cursor_x = 0;
        view->window->win_chat_cursor_y = 0;
    }
    else
    {
        view->window->win_chat_x = 0;
        view->window->win_chat_y = 1;
        view->window->win_chat_width = COLS;
        if (cfg_look_infobar)
            view->window->win_chat_height = LINES - 4;
        else
            view->window->win_chat_height = LINES - 3;
        view->window->win_chat_cursor_x = 0;
        view->window->win_chat_cursor_y = 0;
        view->window->win_nick_x = -1;
        view->window->win_nick_y = -1;
        view->window->win_nick_width = -1;
        view->window->win_nick_height = -1;
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
}

/*
 * gui_draw_view_title: draw title window for a view
 */

void
gui_draw_view_title (t_gui_view *view)
{
    char format[32];
    
    /* TODO: manage splited windows! */
    if (view != gui_current_view)
        return;
    
    if (has_colors ())
    {
        gui_window_set_color (view->window->win_title, COLOR_WIN_TITLE);
        wborder (view->window->win_title, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        wrefresh (view->window->win_title);
        refresh ();
    }
    if (CHANNEL(view))
    {
        snprintf (format, 32, "%%-%ds", view->window->win_width);
        if (CHANNEL(view)->topic)
            mvwprintw (view->window->win_title, 0, 0, format,
                       CHANNEL(view)->topic);
    }
    else
    {
        /* TODO: change this copyright as title? */
        mvwprintw (view->window->win_title, 0, 0,
                   "%s", PACKAGE_STRING " - " WEECHAT_WEBSITE);
        mvwprintw (view->window->win_title, 0, COLS - strlen (WEECHAT_COPYRIGHT),
                   "%s", WEECHAT_COPYRIGHT);
    }
    wrefresh (view->window->win_title);
    refresh ();
}

/*
 * gui_redraw_view_title: redraw title window for a view
 */

void
gui_redraw_view_title (t_gui_view *view)
{
    /* TODO: manage splited windows! */
    if (view != gui_current_view)
        return;
    
    gui_curses_window_clear (view->window->win_title);
    gui_draw_view_title (view);
}

/*
 * gui_get_line_num_splits: returns number of lines on window
 *                          (depending on window width and type (server/channel)
 *                          for alignment)
 */

int
gui_get_line_num_splits (t_gui_view *view, t_gui_line *line)
{
    int length, width;
    
    /* TODO: modify arbitraty value for non aligning messages on time/nick? */
    if (line->length_align >= view->window->win_chat_width - 5)
    {
        length = line->length;
        width = view->window->win_chat_width;
    }
    else
    {
        length = line->length - line->length_align;
        width = view->window->win_chat_width - line->length_align;
    }
    
    return (length % width == 0) ? (length / width) : ((length / width) + 1);
}

/*
 * gui_display_end_of_line: display end of a line in the chat window
 */

void
gui_display_end_of_line (t_gui_view *view, t_gui_line *line, int count)
{
    int lines_displayed, num_lines, offset, remainder, num_displayed;
    t_gui_message *ptr_message;
    char saved_char, format_align[32], format_empty[32];
    
    snprintf (format_align, 32, "%%-%ds", line->length_align);
    num_lines = gui_get_line_num_splits (view, line);
    ptr_message = line->messages;
    offset = 0;
    lines_displayed = 0;
    while (ptr_message)
    {
        /* set text color if beginning of message */
        if (offset == 0)
            gui_window_set_color (view->window->win_chat, ptr_message->color);
        
        /* insert spaces for align text under time/nick */
        if ((lines_displayed > 0) && (view->window->win_chat_cursor_x == 0))
        {
            if (lines_displayed >= num_lines - count)
                mvwprintw (view->window->win_chat,
                           view->window->win_chat_cursor_y,
                           view->window->win_chat_cursor_x,
                           format_align, " ");
            view->window->win_chat_cursor_x += line->length_align;
        }
        
        remainder = strlen (ptr_message->message + offset);
        if (view->window->win_chat_cursor_x + remainder >
            view->window->win_chat_width - 1)
        {
            num_displayed = view->window->win_chat_width -
                view->window->win_chat_cursor_x;
            saved_char = ptr_message->message[offset + num_displayed];
            ptr_message->message[offset + num_displayed] = '\0';
            if (lines_displayed >= num_lines - count)
                mvwprintw (view->window->win_chat,
                           view->window->win_chat_cursor_y, 
                           view->window->win_chat_cursor_x,
                           "%s", ptr_message->message + offset);
            ptr_message->message[offset + num_displayed] = saved_char;
            offset += num_displayed;
        }
        else
        {
            num_displayed = remainder;
            if (lines_displayed >= num_lines - count)
                mvwprintw (view->window->win_chat,
                           view->window->win_chat_cursor_y, 
                           view->window->win_chat_cursor_x,
                           "%s", ptr_message->message + offset);
            ptr_message = ptr_message->next_message;
            offset = 0;
        }
        view->window->win_chat_cursor_x += num_displayed;
        if (!ptr_message ||
            (view->window->win_chat_cursor_x > (view->window->win_chat_width - 1)))
        {
            if (lines_displayed >= num_lines - count)
            {
                if (view->window->win_chat_cursor_x <= view->window->win_chat_width - 1)
                {
                    snprintf (format_empty, 32, "%%-%ds",
                              view->window->win_chat_width - view->window->win_chat_cursor_x);
                    wprintw (view->window->win_chat, format_empty, " ");
                }
                view->window->win_chat_cursor_y++;
            }
            view->window->win_chat_cursor_x = 0;
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
gui_display_line (t_gui_view *view, t_gui_line *line, int stop_at_end)
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
        if (view->window->win_chat_cursor_y > view->window->win_chat_height - 1)
        {
            /*if (!stop_at_end)
                wscrl (view->window->win_chat, +1);*/
            view->window->win_chat_cursor_x = 0;
            view->window->win_chat_cursor_y = view->window->win_chat_height - 1;
            if (stop_at_end)
                return 0;
            view->first_line_displayed = 0;
        }
        
        /* set text color if beginning of message */
        if (offset == 0)
            gui_window_set_color (view->window->win_chat, ptr_message->color);
        
        /* insert spaces for align text under time/nick */
        if ((view->window->win_chat_cursor_x == 0) &&
            (ptr_message->type != MSG_TYPE_TIME) &&
            (ptr_message->type != MSG_TYPE_NICK) &&
            (line->length_align > 0) &&
            /* TODO: modify arbitraty value for non aligning messages on time/nick? */
            (line->length_align < (view->window->win_chat_width - 5)))
        {
            mvwprintw (view->window->win_chat,
                       view->window->win_chat_cursor_y, 
                       view->window->win_chat_cursor_x,
                       format_align, " ");
            view->window->win_chat_cursor_x += line->length_align;
        }
        
        remainder = strlen (ptr_message->message + offset);
        if (view->window->win_chat_cursor_x + remainder > view->window->win_chat_width)
        {
            num_displayed = view->window->win_chat_width -
                view->window->win_chat_cursor_x;
            saved_char = ptr_message->message[offset + num_displayed];
            ptr_message->message[offset + num_displayed] = '\0';
            mvwprintw (view->window->win_chat,
                       view->window->win_chat_cursor_y, 
                       view->window->win_chat_cursor_x,
                       "%s", ptr_message->message + offset);
            ptr_message->message[offset + num_displayed] = saved_char;
            offset += num_displayed;
        }
        else
        {
            num_displayed = remainder;
            mvwprintw (view->window->win_chat,
                       view->window->win_chat_cursor_y, 
                       view->window->win_chat_cursor_x,
                       "%s", ptr_message->message + offset);
            offset = 0;
            ptr_message = ptr_message->next_message;
        }
        view->window->win_chat_cursor_x += num_displayed;
        if (!ptr_message ||
            (view->window->win_chat_cursor_x > (view->window->win_chat_width - 1)))
        {
            if (!ptr_message ||
                ((view->window->win_chat_cursor_y <= view->window->win_chat_height - 1) &&
                (view->window->win_chat_cursor_x > view->window->win_chat_width - 1)))
            {
                if (view->window->win_chat_cursor_x <= view->window->win_chat_width - 1)
                {
                    snprintf (format_empty, 32, "%%-%ds",
                              view->window->win_chat_width - view->window->win_chat_cursor_x);
                    wprintw (view->window->win_chat, format_empty, " ");
                }
                view->window->win_chat_cursor_y++;
            }
            view->window->win_chat_cursor_x = 0;
        }
    }
    return 1;
}

/*
 * gui_draw_view_chat: draw chat window for a view
 */

void
gui_draw_view_chat (t_gui_view *view)
{
    t_gui_line *ptr_line;
    int lines_used;
    
    /* TODO: manage splited windows! */
    if (view != gui_current_view)
        return;
    
    if (has_colors ())
        gui_window_set_color (view->window->win_chat, COLOR_WIN_CHAT);
    
    ptr_line = view->last_line;
    lines_used = 0;
    while (ptr_line
           && (lines_used < (view->window->win_chat_height + view->sub_lines)))
    {
        lines_used += gui_get_line_num_splits (view, ptr_line);
        ptr_line = ptr_line->prev_line;
    }
    view->window->win_chat_cursor_x = 0;
    view->window->win_chat_cursor_y = 0;
    if (lines_used > (view->window->win_chat_height + view->sub_lines))
    {
        /* screen will be full (we'll display only end of 1st line) */
        ptr_line = (ptr_line) ? ptr_line->next_line : view->lines;
        gui_display_end_of_line (view, ptr_line,
                                 gui_get_line_num_splits (view, ptr_line) -
                                 (lines_used - (view->window->win_chat_height + view->sub_lines)));
        ptr_line = ptr_line->next_line;
        view->first_line_displayed = 0;
    }
    else
    {
        /* all lines are displayed */
        if (!ptr_line)
        {
            view->first_line_displayed = 1;
            ptr_line = view->lines;
        }
        else
        {
            view->first_line_displayed = 0;
            ptr_line = ptr_line->next_line;
        }
    }
    while (ptr_line)
    {
        if (!gui_display_line (view, ptr_line, 1))
            break;
  
        ptr_line = ptr_line->next_line;
    }
    /*if (view->window->win_chat_cursor_y <= view->window->win_chat_height - 1)
        view->sub_lines = 0;*/
    wrefresh (view->window->win_chat);
    refresh ();
}

/*
 * gui_redraw_view_chat: redraw chat window for a view
 */

void
gui_redraw_view_chat (t_gui_view *view)
{
    char format_empty[32];
    int i;
    
    /* TODO: manage splited windows! */
    if (view != gui_current_view)
        return;
    
    if (has_colors ())
        gui_window_set_color (view->window->win_chat, COLOR_WIN_CHAT);
    
    snprintf (format_empty, 32, "%%-%ds", view->window->win_chat_width);
    for (i = 0; i < view->window->win_chat_height; i++)
    {
        mvwprintw (view->window->win_chat, i, 0, format_empty, " ");
    }
    
    gui_draw_view_chat (view);
}

/*
 * gui_draw_view_nick: draw nick window for a view
 */

void
gui_draw_view_nick (t_gui_view *view)
{
    int i, x, y, column, max_length;
    char format[32], format_empty[32];
    t_irc_nick *ptr_nick;
    
    /* TODO: manage splited windows! */
    if (view != gui_current_view)
        return;
    
    if (CHANNEL(view) && CHANNEL(view)->nicks)
    {
        max_length = nick_get_max_length (CHANNEL(view));
        if ((view == gui_current_view) &&
            ((max_length + 2) != view->window->win_nick_width))
        {
            gui_calculate_pos_size (view);
            delwin (view->window->win_chat);
            delwin (view->window->win_nick);
            view->window->win_chat = newwin (view->window->win_chat_height,
                                             view->window->win_chat_width,
                                             view->window->win_chat_y,
                                             view->window->win_chat_x);
            view->window->win_nick = newwin (view->window->win_nick_height,
                                             view->window->win_nick_width,
                                             view->window->win_nick_y,
                                             view->window->win_nick_x);
            gui_redraw_view_chat (view);
            
            if (has_colors ())
                gui_window_set_color (view->window->win_nick, COLOR_WIN_NICK);
            
            snprintf (format_empty, 32, "%%-%ds", view->window->win_nick_width);
            for (i = 0; i < view->window->win_nick_height; i++)
            {
                mvwprintw (view->window->win_nick, i, 0, format_empty, " ");
            }
        }
        snprintf (format, 32, "%%-%ds", max_length);
            
        if (has_colors ())
        {
            switch (cfg_look_nicklist_position)
            {
                case CFG_LOOK_NICKLIST_LEFT:
                    gui_window_set_color (view->window->win_nick, COLOR_WIN_NICK_SEP);
                    for (i = 0; i < view->window->win_chat_height; i++)
                        mvwprintw (view->window->win_nick,
                                   i, view->window->win_nick_width - 1, " ");
                    break;
                case CFG_LOOK_NICKLIST_RIGHT:
                    gui_window_set_color (view->window->win_nick, COLOR_WIN_NICK_SEP);
                    for (i = 0; i < view->window->win_chat_height; i++)
                        mvwprintw (view->window->win_nick,
                                   i, 0, " ");
                    break;
                case CFG_LOOK_NICKLIST_TOP:
                    gui_window_set_color (view->window->win_nick, COLOR_WIN_NICK);
                    for (i = 0; i < view->window->win_chat_width; i += 2)
                        mvwprintw (view->window->win_nick,
                                   view->window->win_nick_height - 1, i, "-");
                    break;
                case CFG_LOOK_NICKLIST_BOTTOM:
                    gui_window_set_color (view->window->win_nick, COLOR_WIN_NICK);
                    for (i = 0; i < view->window->win_chat_width; i += 2)
                        mvwprintw (view->window->win_nick,
                                   0, i, "-");
                    break;
            }
        }
    
        gui_window_set_color (view->window->win_nick, COLOR_WIN_NICK);
        x = 0;
        y = (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_BOTTOM) ? 1 : 0;
        column = 0;
        for (ptr_nick = CHANNEL(view)->nicks; ptr_nick;
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
                gui_window_set_color (view->window->win_nick, COLOR_WIN_NICK_OP);
                mvwprintw (view->window->win_nick, y, x, "@");
                x++;
            }
            else
            {
                if (ptr_nick->is_halfop)
                {
                    gui_window_set_color (view->window->win_nick, COLOR_WIN_NICK_HALFOP);
                    mvwprintw (view->window->win_nick, y, x, "%%");
                    x++;
                }
                else
                {
                    if (ptr_nick->has_voice)
                    {
                        gui_window_set_color (view->window->win_nick, COLOR_WIN_NICK_VOICE);
                        mvwprintw (view->window->win_nick, y, x, "+");
                        x++;
                    }
                    else
                    {
                        gui_window_set_color (view->window->win_nick, COLOR_WIN_NICK);
                        mvwprintw (view->window->win_nick, y, x, " ");
                        x++;
                    }
                }
            }
            gui_window_set_color (view->window->win_nick, COLOR_WIN_NICK);
            mvwprintw (view->window->win_nick, y, x, format, ptr_nick->nick);
            y++;
            if ((cfg_look_nicklist_position == CFG_LOOK_NICKLIST_TOP) ||
                (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_BOTTOM))
            {
                if (y - ((cfg_look_nicklist_position == CFG_LOOK_NICKLIST_BOTTOM) ? 1 : 0) >= view->window->win_nick_height - 1)
                {
                    column += max_length + 2;
                    y = (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_TOP) ?
                        0 : 1;
                }
            }
        }
    }
    wrefresh (view->window->win_nick);
    refresh ();
}

/*
 * gui_redraw_view_nick: redraw nick window for a view
 */

void
gui_redraw_view_nick (t_gui_view *view)
{
    char format_empty[32];
    int i;
    
    /* TODO: manage splitted windows! */
    if (view != gui_current_view)
        return;
    
    if (has_colors ())
        gui_window_set_color (view->window->win_nick, COLOR_WIN_NICK);
    
    snprintf (format_empty, 32, "%%-%ds", view->window->win_nick_width);
    for (i = 0; i < view->window->win_nick_height; i++)
    {
        mvwprintw (view->window->win_nick, i, 0, format_empty, " ");
    }
    
    gui_draw_view_nick (view);
}

/*
 * gui_draw_view_status: draw status window for a view
 */

void
gui_draw_view_status (t_gui_view *view)
{
    t_gui_view *ptr_view;
    char format_more[32];
    int i, first_mode;
    
    /* TODO: manage splitted windows! */
    if (view != gui_current_view)
        return;
    
    if (has_colors ())
    {
        gui_window_set_color (view->window->win_status, COLOR_WIN_STATUS);
        wborder (view->window->win_status, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        wrefresh (view->window->win_status);
    }
    wmove (view->window->win_status, 0, 0);
    for (ptr_view = gui_views; ptr_view; ptr_view = ptr_view->next_view)
    {
        if (SERVER(ptr_view) && !CHANNEL(ptr_view))
        {
            if (gui_current_view == SERVER(ptr_view)->view)
            {
                if (ptr_view->unread_data)
                {
                    if (ptr_view->unread_data > 1)
                        gui_window_set_color (view->window->win_status,
                                              COLOR_WIN_STATUS_DATA_MSG);
                    else
                        gui_window_set_color (view->window->win_status,
                                              COLOR_WIN_STATUS_DATA_OTHER);
                }
                else
                    gui_window_set_color (view->window->win_status,
                                          COLOR_WIN_STATUS_ACTIVE);
            }
            else
            {
                if (SERVER(ptr_view)->view &&
                    ((SERVER(ptr_view)->view)->unread_data))
                {
                    if (SERVER(ptr_view)->view->unread_data > 1)
                        gui_window_set_color (view->window->win_status,
                                              COLOR_WIN_STATUS_DATA_MSG);
                    else
                        gui_window_set_color (view->window->win_status,
                                              COLOR_WIN_STATUS_DATA_OTHER);
                }
                else
                    gui_window_set_color (view->window->win_status,
                                          COLOR_WIN_STATUS);
            }
            if (SERVER(ptr_view)->is_connected)
                wprintw (view->window->win_status, "[%s] ",
                         SERVER(ptr_view)->name);
            else
                wprintw (view->window->win_status, "(%s) ",
                         SERVER(ptr_view)->name);
        }
        if (SERVER(ptr_view) && CHANNEL(ptr_view))
        {
            if (gui_current_view == CHANNEL(ptr_view)->view)
            {
                if ((CHANNEL(ptr_view)->view) &&
                    (CHANNEL(ptr_view)->view->unread_data))
                {
                    if (CHANNEL(ptr_view)->view->unread_data > 1)
                        gui_window_set_color (view->window->win_status,
                                              COLOR_WIN_STATUS_DATA_MSG);
                    else
                        gui_window_set_color (view->window->win_status,
                                              COLOR_WIN_STATUS_DATA_OTHER);
                }
                else
                    gui_window_set_color (view->window->win_status,
                                          COLOR_WIN_STATUS_ACTIVE);
            }
            else
            {
                if ((CHANNEL(ptr_view)->view) &&
                    (CHANNEL(ptr_view)->view->unread_data))
                {
                    if (CHANNEL(ptr_view)->view->unread_data > 1)
                        gui_window_set_color (view->window->win_status,
                                              COLOR_WIN_STATUS_DATA_MSG);
                    else
                        gui_window_set_color (view->window->win_status,
                                              COLOR_WIN_STATUS_DATA_OTHER);
                }
                else
                    gui_window_set_color (view->window->win_status,
                                          COLOR_WIN_STATUS);
            }
            wprintw (view->window->win_status, "%s", CHANNEL(ptr_view)->name);
            if (gui_current_view == CHANNEL(ptr_view)->view)
            {
                /* display channel modes */
                wprintw (view->window->win_status, "(");
                i = 0;
                first_mode = 1;
                while (CHANNEL(ptr_view)->modes[i])
                {
                    if (CHANNEL(ptr_view)->modes[i] != ' ')
                    {
                        if (first_mode)
                        {
                            wprintw (view->window->win_status, "+");
                            first_mode = 0;
                        }
                        wprintw (view->window->win_status, "%c",
                                 CHANNEL(ptr_view)->modes[i]);
                    }
                    i++;
                }
                if (CHANNEL(ptr_view)->modes[CHANNEL_MODE_KEY] != ' ')
                    wprintw (view->window->win_status, ",%s",
                             CHANNEL(ptr_view)->key);
                if (CHANNEL(ptr_view)->modes[CHANNEL_MODE_LIMIT] != ' ')
                    wprintw (view->window->win_status, ",%d",
                             CHANNEL(ptr_view)->limit);
                wprintw (view->window->win_status, ")");
            }
            wprintw (view->window->win_status, " ");
        }
        if (!SERVER(ptr_view))
        {
            gui_window_set_color (view->window->win_status, COLOR_WIN_STATUS);
            wprintw (view->window->win_status, _("[not connected] "));
        }
    }
    
    /* display "-MORE-" if last line is not displayed */
    gui_window_set_color (view->window->win_status, COLOR_WIN_STATUS_MORE);
    if (view->sub_lines > 0)
        mvwprintw (view->window->win_status, 0, COLS - 7, _("-MORE-"));
    else
    {
        snprintf (format_more, 32, "%%-%ds", strlen (_("-MORE-")));
        mvwprintw (view->window->win_status, 0, COLS - 7, format_more, " ");
    }
    
    wrefresh (view->window->win_status);
    refresh ();
}

/*
 * gui_redraw_view_status: redraw status window for a view
 */

void
gui_redraw_view_status (t_gui_view *view)
{
    /* TODO: manage splited windows! */
    if (view != gui_current_view)
        return;
    
    gui_curses_window_clear (view->window->win_status);
    gui_draw_view_status (view);
}

/*
 * gui_draw_view_infobar: draw infobar window for a view
 */

void
gui_draw_view_infobar (t_gui_view *view)
{
    time_t time_seconds;
    struct tm *local_time;
    char text[1024 + 1];
    
    /* TODO: manage splited windows! */
    if (view != gui_current_view)
        return;
    
    if (has_colors ())
    {
        gui_window_set_color (view->window->win_infobar, COLOR_WIN_INFOBAR);
        wborder (view->window->win_infobar, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        wrefresh (view->window->win_infobar);
    }
    wmove (view->window->win_infobar, 0, 0);
    
    time_seconds = time (NULL);
    local_time = localtime (&time_seconds);
    if (local_time)
    {
        strftime (text, 1024, cfg_look_infobar_timestamp, local_time);
        gui_window_set_color (view->window->win_infobar, COLOR_WIN_INFOBAR);
        wprintw (view->window->win_infobar, "%s", text);
    }
    if (gui_infobar)
    {
        gui_window_set_color (view->window->win_infobar, gui_infobar->color);
        wprintw (view->window->win_infobar, " | %s", gui_infobar->text);
    }
    
    wrefresh (view->window->win_infobar);
    refresh ();
}

/*
 * gui_redraw_view_infobar: redraw infobar window for a view
 */

void
gui_redraw_view_infobar (t_gui_view *view)
{
    /* TODO: manage splited windows! */
    if (view != gui_current_view)
        return;
    
    gui_curses_window_clear (view->window->win_infobar);
    gui_draw_view_infobar (view);
}

/*
 * gui_get_input_width: return input width (max # chars displayed)
 */

int
gui_get_input_width (t_gui_view *view)
{
    if (CHANNEL(view))
        return (COLS - strlen (CHANNEL(view)->name) -
                strlen (SERVER(view)->nick) - 3);
    else
    {
        if (SERVER(view) && (SERVER(view)->is_connected))
            return (COLS - strlen (SERVER(view)->nick) - 2);
        else
            return (COLS - strlen (cfg_look_no_nickname) - 2);
    }
}

/*
 * gui_draw_view_input: draw input window for a view
 */

void
gui_draw_view_input (t_gui_view *view)
{
    char format[32];
    char *ptr_nickname;
    int input_width;
    
    /* TODO: manage splited windows! */
    if (view != gui_current_view)
        return;
    
    if (has_colors ())
    {
        gui_window_set_color (view->window->win_input, COLOR_WIN_INPUT);
        wborder (view->window->win_input, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        wrefresh (view->window->win_input);
    }
    
    if (view->input_buffer_size == 0)
        view->input_buffer[0] = '\0';
    
    input_width = gui_get_input_width (view);
    
    if (view->input_buffer_pos - view->input_buffer_1st_display + 1 >
        input_width)
        view->input_buffer_1st_display = view->input_buffer_pos -
            input_width + 1;
    else
    {
        if (view->input_buffer_pos < view->input_buffer_1st_display)
            view->input_buffer_1st_display = view->input_buffer_pos;
        else
        {
            if ((view->input_buffer_1st_display > 0) &&
                (view->input_buffer_pos -
                view->input_buffer_1st_display + 1) < input_width)
            {
                view->input_buffer_1st_display =
                    view->input_buffer_pos - input_width + 1;
                if (view->input_buffer_1st_display < 0)
                    view->input_buffer_1st_display = 0;
            }
        }
    }
    if (CHANNEL(view))
    {
        snprintf (format, 32, "%%s %%s> %%-%ds", input_width);
        mvwprintw (view->window->win_input, 0, 0, format,
                   CHANNEL(view)->name,
                   SERVER(view)->nick,
                   view->input_buffer + view->input_buffer_1st_display);
        wclrtoeol (view->window->win_input);
        move (LINES - 1, strlen (CHANNEL(view)->name) +
              strlen (SERVER(view)->nick) + 3 +
              (view->input_buffer_pos - view->input_buffer_1st_display));
    }
    else
    {
        if (SERVER(view))
        {
            snprintf (format, 32, "%%s> %%-%ds", input_width);
            if (SERVER(view) && (SERVER(view)->is_connected))
                ptr_nickname = SERVER(view)->nick;
            else
                ptr_nickname = cfg_look_no_nickname;
            mvwprintw (view->window->win_input, 0, 0, format,
                       ptr_nickname,
                       view->input_buffer + view->input_buffer_1st_display);
            wclrtoeol (view->window->win_input);
            move (LINES - 1, strlen (ptr_nickname) + 2 +
                  (view->input_buffer_pos - view->input_buffer_1st_display));
        }
        else
        {
            snprintf (format, 32, "%%s> %%-%ds", input_width);
            if (SERVER(view) && (SERVER(view)->is_connected))
                ptr_nickname = SERVER(view)->nick;
            else
                ptr_nickname = cfg_look_no_nickname;
            mvwprintw (view->window->win_input, 0, 0, format,
                       ptr_nickname,
                       view->input_buffer + view->input_buffer_1st_display);
            wclrtoeol (view->window->win_input);
            move (LINES - 1, strlen (ptr_nickname) + 2 +
                  (view->input_buffer_pos - view->input_buffer_1st_display));
        }
    }
    
    wrefresh (view->window->win_input);
    refresh ();
}

/*
 * gui_redraw_view_input: redraw input window for a view
 */

void
gui_redraw_view_input (t_gui_view *view)
{
    /* TODO: manage splited windows! */
    if (view != gui_current_view)
        return;
    
    gui_curses_window_clear (view->window->win_input);
    gui_draw_view_input (view);
}

/*
 * gui_redraw_view: redraw a view
 */

void
gui_redraw_view (t_gui_view *view)
{
    /* TODO: manage splited windows! */
    if (view != gui_current_view)
        return;
    
    gui_redraw_view_title (view);
    gui_redraw_view_chat (view);
    if (view->window->win_nick)
        gui_redraw_view_nick (view);
    gui_redraw_view_status (view);
    if (cfg_look_infobar)
        gui_redraw_view_infobar (view);
    gui_redraw_view_input (view);
}

/*
 * gui_switch_to_view: switch to another view
 */

void
gui_switch_to_view (t_gui_view *view)
{
    int another_view;
    t_gui_view *ptr_view;
    
    another_view = 0;
    /*for (ptr_view = gui_views; ptr_view; ptr_view = ptr_view->next_view)
    {
        if (ptr_view->is_displayed)
        {*/
            /* TODO: manage splited windows */
            /*another_view = 1;
            view->window->win_title = ptr_view->window->win_title;
            view->window->win_chat = ptr_view->window->win_chat;
            view->window->win_nick = ptr_view->window->win_nick;
            view->window->win_status = ptr_view->window->win_status;
            view->window->win_infobar = ptr_view->window->win_infobar;
            view->window->win_input = ptr_view->window->win_input;
            if (ptr_view != view)
            {
                ptr_view->window->win_title = NULL;
                ptr_view->window->win_chat = NULL;
                ptr_view->window->win_nick = NULL;
                ptr_view->window->win_status = NULL;
                ptr_view->window->win_infobar = NULL;
                ptr_view->window->win_input = NULL;
                ptr_view->is_displayed = 0;
            }
            break;
        }
    }*/
    
    gui_calculate_pos_size (view);
    
    /* first time creation for views */
    if (!another_view)
    {
        /* create new views */
        view->window->win_title = newwin (1, COLS, 0, 0);
        view->window->win_chat = newwin (view->window->win_chat_height,
                                         view->window->win_chat_width,
                                         view->window->win_chat_y,
                                         view->window->win_chat_x);
        if (cfg_look_nicklist && CHANNEL(view))
            view->window->win_nick = newwin (view->window->win_nick_height,
                                             view->window->win_nick_width,
                                             view->window->win_nick_y,
                                             view->window->win_nick_x);
        else
            view->window->win_nick = NULL;
        view->window->win_input = newwin (1, COLS, LINES - 1, 0);
    }
    else
    {
        /* remove some views */
        if (view->window->win_nick)
        {
            delwin (view->window->win_nick);
            view->window->win_nick = NULL;
        }
        if (view->window->win_status)
        {
            delwin (view->window->win_status);
            view->window->win_status = NULL;
        }
        if (view->window->win_infobar)
        {
            delwin (view->window->win_infobar);
            view->window->win_infobar = NULL;
        }
        
        /* create views */
        if (VIEW_IS_CHANNEL(view))
        {
            delwin (view->window->win_chat);
            view->window->win_chat = newwin (view->window->win_chat_height,
                                             view->window->win_chat_width,
                                             view->window->win_chat_y,
                                             view->window->win_chat_x);
            if (cfg_look_nicklist)
                view->window->win_nick = newwin (view->window->win_nick_height,
                                                 view->window->win_nick_width,
                                                 view->window->win_nick_y,
                                                 view->window->win_nick_x);
            else
                view->window->win_nick = NULL;
        }
        if (!(VIEW_IS_CHANNEL(view)))
        {
            delwin (view->window->win_chat);
            view->window->win_chat = newwin (view->window->win_chat_height,
                                             view->window->win_chat_width,
                                             view->window->win_chat_y,
                                             view->window->win_chat_x);
        }
    }
    
    /* create status/infobar windows */
    if (cfg_look_infobar)
    {
        view->window->win_infobar = newwin (1, COLS, LINES - 2, 0);
        view->window->win_status = newwin (1, COLS, LINES - 3, 0);
    }
    else
        view->window->win_status = newwin (1, COLS, LINES - 2, 0);
    
    /* change current view to the new view */
    gui_current_view = view;
    
    view->is_displayed = 1;
    view->unread_data = 0;
}

/*
 * gui_switch_to_previous_view: switch to previous view
 */

void
gui_switch_to_previous_view ()
{
    /* if only one view then return */
    if (gui_views == last_gui_view)
        return;
    
    if (gui_current_view->prev_view)
        gui_switch_to_view (gui_current_view->prev_view);
    else
        gui_switch_to_view (last_gui_view);
    gui_redraw_view (gui_current_view);
}

/*
 * gui_switch_to_next_view: switch to next view
 */

void
gui_switch_to_next_view ()
{
    /* if only one view then return */
    if (gui_views == last_gui_view)
        return;
    
    if (gui_current_view->next_view)
        gui_switch_to_view (gui_current_view->next_view);
    else
        gui_switch_to_view (gui_views);
    gui_redraw_view (gui_current_view);
}

/*
 * gui_move_page_up: display previous page on view
 */

void
gui_move_page_up ()
{
    if (!gui_current_view->first_line_displayed)
    {
        gui_current_view->sub_lines += gui_current_view->window->win_chat_height - 1;
        gui_draw_view_chat (gui_current_view);
        gui_draw_view_status (gui_current_view);
    }
}

/*
 * gui_move_page_down: display next page on view
 */

void
gui_move_page_down ()
{
    if (gui_current_view->sub_lines > 0)
    {
        gui_current_view->sub_lines -= gui_current_view->window->win_chat_height - 1;
        if (gui_current_view->sub_lines < 0)
            gui_current_view->sub_lines = 0;
        if (gui_current_view->sub_lines == 0)
            gui_current_view->unread_data = 0;
        gui_draw_view_chat (gui_current_view);
        gui_draw_view_status (gui_current_view);
    }
}

/*
 * gui_curses_resize_handler: called when term size is modified
 */

void
gui_curses_resize_handler ()
{
    t_gui_view *ptr_view;
    int width, height;
    
    endwin ();
    refresh ();
    
    getmaxyx (stdscr, height, width);
    
    for (ptr_view = gui_views; ptr_view; ptr_view = ptr_view->next_view)
    {
        // TODO: manage splited windows!
        if (ptr_view->window->win_title)
        {
            ptr_view->is_displayed = 0;
            if (ptr_view->window->win_title)
                delwin (ptr_view->window->win_title);
            if (ptr_view->window->win_chat)
                delwin (ptr_view->window->win_chat);
            if (ptr_view->window->win_nick)
                delwin (ptr_view->window->win_nick);
            if (ptr_view->window->win_status)
                delwin (ptr_view->window->win_status);
            if (ptr_view->window->win_infobar)
                delwin (ptr_view->window->win_infobar);
            if (ptr_view->window->win_input)
                delwin (ptr_view->window->win_input);
            ptr_view->window->win_title = NULL;
            ptr_view->window->win_chat = NULL;
            ptr_view->window->win_nick = NULL;
            ptr_view->window->win_status = NULL;
            ptr_view->window->win_infobar = NULL;
            ptr_view->window->win_input = NULL;
            gui_switch_to_view (ptr_view);
        }
    }
}

/*
 * gui_view_init_subviews: init subviews for a WeeChat view
 */

void
gui_view_init_subviews (t_gui_view *view)
{
    view->window->win_title = NULL;
    view->window->win_chat = NULL;
    view->window->win_nick = NULL;
    view->window->win_status = NULL;
    view->window->win_infobar = NULL;
    view->window->win_input = NULL;
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
        init_pair (COLOR_WIN_STATUS_ACTIVE,
            cfg_col_status_active & A_CHARTEXT, cfg_col_status_bg);
        init_pair (COLOR_WIN_STATUS_DATA_MSG,
            cfg_col_status_data_msg & A_CHARTEXT, cfg_col_status_bg);
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
        color_attr[COLOR_WIN_STATUS_ACTIVE - 1] = cfg_col_status_active & A_BOLD;
        color_attr[COLOR_WIN_STATUS_DATA_MSG - 1] = cfg_col_status_data_msg & A_BOLD;
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
    
    /* create a new view */
    if ((gui_windows = gui_window_new (0, 0, COLS, LINES)))
    {
        gui_current_view = gui_view_new (gui_windows, NULL, NULL, 1 /*0, 0, COLS, LINES*/);
    
        signal (SIGWINCH, gui_curses_resize_handler);
    
        if (cfg_look_set_title)
            gui_set_window_title ();
    
        gui_ready = 1;
    }
}

/*
 * gui_end: GUI end
 */

void
gui_end ()
{
    t_gui_view *ptr_view;
    
    /* delete all views */
    for (ptr_view = gui_views; ptr_view; ptr_view = ptr_view->next_view)
    {
        if (ptr_view->window->win_title)
            delwin (ptr_view->window->win_title);
        if (ptr_view->window->win_chat)
            delwin (ptr_view->window->win_chat);
        if (ptr_view->window->win_nick)
            delwin (ptr_view->window->win_nick);
        if (ptr_view->window->win_status)
            delwin (ptr_view->window->win_status);
        if (ptr_view->window->win_infobar)
            delwin (ptr_view->window->win_infobar);
        if (ptr_view->window->win_input)
            delwin (ptr_view->window->win_input);
        /* TODO: free input buffer, lines, messages, completion */
    }
    
    /* end of curses output */
    refresh ();
    endwin ();
}

/*
 * gui_add_message: add a message to a view
 */

void
gui_add_message (t_gui_view *view, int type, int color, char *message)
{
    char *pos;
    int length;
    
    /* create new line if previous was ending by '\n' (or if 1st line) */
    if (view->line_complete)
    {
        view->line_complete = 0;
        if (!gui_new_line (view))
            return;
    }
    if (!gui_new_message (view))
        return;
    
    view->last_line->last_message->type = type;
    view->last_line->last_message->color = color;
    pos = strchr (message, '\n');
    if (pos)
    {
        pos[0] = '\0';
        view->line_complete = 1;
    }
    view->last_line->last_message->message = strdup (message);
    length = strlen (message);
    view->last_line->length += length;
    if (type == MSG_TYPE_MSG)
        view->last_line->line_with_message = 1;
    if ((type == MSG_TYPE_TIME) || (type == MSG_TYPE_NICK))
        view->last_line->length_align += length;
    if (pos)
    {
        pos[0] = '\n';
        if ((view == gui_current_view) && (view->sub_lines == 0))
        {
            if ((view->window->win_chat_cursor_y
                + gui_get_line_num_splits (view, view->last_line)) >
                (view->window->win_chat_height - 1))
                gui_draw_view_chat (view);
            else
                gui_display_line (view, view->last_line, 1);
        }
        if ((view != gui_current_view) || (view->sub_lines > 0))
        {
            if (view->unread_data < 1 + view->last_line->line_with_message)
            {
                view->unread_data = 1 + view->last_line->line_with_message;
                gui_redraw_view_status (gui_current_view);
            }
        }
    }
}

/*
 * gui_printf_color_type: display a message in a view
 */

void
gui_printf_color_type (t_gui_view *view, int type, int color, char *message, ...)
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
    
        if (view == NULL)
        {
            if (SERVER(gui_current_view))
                view = SERVER(gui_current_view)->view;
            else
                view = gui_current_view;
        }
    
        if (view == NULL)
        {
            wee_log_printf ("gui_printf without view! this is a bug, please send to developers - thanks\n");
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
            if ((!view->last_line) || (view->line_complete))
            {
                gui_add_message (view, MSG_TYPE_TIME, COLOR_WIN_CHAT_DARK, "[");
                snprintf (timestamp, 16, "%02d", date_tmp->tm_hour);
                gui_add_message (view, MSG_TYPE_TIME, COLOR_WIN_CHAT_TIME, timestamp);
                gui_add_message (view, MSG_TYPE_TIME, COLOR_WIN_CHAT_TIME_SEP, ":");
                snprintf (timestamp, 16, "%02d", date_tmp->tm_min);
                gui_add_message (view, MSG_TYPE_TIME, COLOR_WIN_CHAT_TIME, timestamp);
                gui_add_message (view, MSG_TYPE_TIME, COLOR_WIN_CHAT_TIME_SEP, ":");
                snprintf (timestamp, 16, "%02d", date_tmp->tm_sec);
                gui_add_message (view, MSG_TYPE_TIME, COLOR_WIN_CHAT_TIME, timestamp);
                gui_add_message (view, MSG_TYPE_TIME, COLOR_WIN_CHAT_DARK, "] ");
            }
            gui_add_message (view, type, color, pos + 1);
            pos = strchr (pos + 1, '\n');
            if (pos && !pos[1])
                pos = NULL;
        }
        
        wrefresh (view->window->win_chat);
        refresh ();
    }
    else
        printf ("%s", buffer);
}
