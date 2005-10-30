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
#include <ctype.h>

#ifdef HAVE_NCURSESW_CURSES_H
#include <ncursesw/ncurses.h>
#else
#include <ncurses.h>
#endif

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../common/weeconfig.h"
#include "../../common/hotlist.h"
#include "../../common/log.h"
#include "../../common/utf8.h"
#include "../../irc/irc.h"


t_gui_color gui_colors[] =
{ { "default",      -1 | A_NORMAL },
  { "black",        COLOR_BLACK | A_NORMAL },
  { "red",          COLOR_RED | A_NORMAL },
  { "lightred",     COLOR_RED | A_BOLD },
  { "green",        COLOR_GREEN | A_NORMAL },
  { "lightgreen",   COLOR_GREEN | A_BOLD },
  { "brown",        COLOR_YELLOW | A_NORMAL },
  { "yellow",       COLOR_YELLOW | A_BOLD },
  { "blue",         COLOR_BLUE | A_NORMAL },
  { "lightblue",    COLOR_BLUE | A_BOLD },
  { "magenta",      COLOR_MAGENTA | A_NORMAL },
  { "lightmagenta", COLOR_MAGENTA | A_BOLD },
  { "cyan",         COLOR_CYAN | A_NORMAL },
  { "lightcyan",    COLOR_CYAN | A_BOLD },
  { "white",        COLOR_WHITE | A_BOLD },
  { NULL, 0 }
};

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
        if (ascii_strcasecmp (gui_colors[i].name, color_name) == 0)
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
        if (ascii_strcasecmp (gui_colors[i].name, color_name) == 0)
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
    if (has_colors ())
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
        
        if ((cfg_look_nicklist_min_size > 0)
            && (max_length < cfg_look_nicklist_min_size))
            max_length = cfg_look_nicklist_min_size;
        else if ((cfg_look_nicklist_max_size > 0)
                 && (max_length > cfg_look_nicklist_max_size))
            max_length = cfg_look_nicklist_max_size;
        
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
 * gui_draw_window_separator: draw window separation
 */

void
gui_draw_window_separator (t_gui_window *window)
{
    if (window->win_separator)
        delwin (window->win_separator);
    
    if (window->win_x > 0)
    {
        window->win_separator = newwin (window->win_height,
                                        1,
                                        window->win_y,
                                        window->win_x - 1);
        gui_window_set_color (window->win_separator, COLOR_WIN_TITLE);
        wborder (window->win_separator, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        wnoutrefresh (window->win_separator);
        refresh ();
    }
}

/*
 * gui_draw_buffer_title: draw title window for a buffer
 */

void
gui_draw_buffer_title (t_gui_buffer *buffer, int erase)
{
    t_gui_window *ptr_win;
    char format[32], *buf;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            if (erase)
              gui_curses_window_clear (ptr_win->win_title);
            
            gui_window_set_color (ptr_win->win_title, COLOR_WIN_TITLE);
            snprintf (format, 32, "%%-%ds", ptr_win->win_width);
            if (CHANNEL(buffer))
            {
                if (CHANNEL(buffer)->topic)
                {
                    buf = weechat_convert_encoding ((local_utf8) ?
                                                    cfg_look_charset_decode_iso : cfg_look_charset_decode_utf,
                                                    (cfg_look_charset_internal && cfg_look_charset_internal[0]) ?
                                                    cfg_look_charset_internal : local_charset,
                                                    CHANNEL(buffer)->topic);
                    mvwprintw (ptr_win->win_title, 0, 0, format, buf);
                    free (buf);
                }
                else
                    mvwprintw (ptr_win->win_title, 0, 0, format, " ");
            }
            else
            {
                if (!buffer->dcc)
                {
                    mvwprintw (ptr_win->win_title, 0, 0,
                               format,
                               PACKAGE_STRING " " WEECHAT_COPYRIGHT_DATE " - "
                               WEECHAT_WEBSITE);
                }
                else
                    mvwprintw (ptr_win->win_title, 0, 0, format, " ");
            }
            wnoutrefresh (ptr_win->win_title);
            refresh ();
        }
    }
}

/*
 * gui_display_new_line: display a new line
 */

void
gui_display_new_line (t_gui_window *window, int num_lines, int count,
                      int *lines_displayed, int simulate)
{
    char format_empty[32];
    
    if ((count == 0) || (*lines_displayed >= num_lines - count))
    {
        if ((!simulate) && (window->win_chat_cursor_x <= window->win_chat_width - 1))
        {
            snprintf (format_empty, 32, "%%-%ds",
                      window->win_chat_width - window->win_chat_cursor_x);
            wprintw (window->win_chat, format_empty, " ");
        }
        window->win_chat_cursor_y++;
    }
    window->win_chat_cursor_x = 0;
    (*lines_displayed)++;
}

/*
 * gui_message_get_next_char: returns next char of message at offset
 */

void
gui_message_get_next_char (t_gui_message **message, int *offset)
{
    if (!(*message))
        return;
    
    (*offset) += utf8_char_size ((*message)->message + (*offset));
    if (!((*message)->message[*offset]))
    {
        *message = (*message)->next_message;
        *offset = 0;
    }
}

/*
 * gui_display_word: display a word on chat buffer
 */

void
gui_display_word (t_gui_window *window, t_gui_line *line,
                  t_gui_message *message, int offset,
                  t_gui_message *end_msg, int end_offset,
                  int num_lines, int count, int *lines_displayed, int simulate)
{
    char format_align[32];
    char saved_char_end, saved_char;
    int pos_saved_char, end_of_word, chars_to_display, num_displayed;
    
    if (!message || !end_msg ||
        ((!simulate) && (window->win_chat_cursor_y > window->win_chat_height - 1)))
        return;
    
    snprintf (format_align, 32, "%%-%ds", line->length_align);
    
    saved_char_end = '\0';
    if (end_msg)
    {
        saved_char_end = end_msg->message[end_offset + 1];
        end_msg->message[end_offset + 1] = '\0';
    }
    
    end_of_word = 0;
    while (!end_of_word)
    {
        /* set text color if beginning of message */
        if (!simulate)
            gui_window_set_color (window->win_chat, message->color);
        
        /* insert spaces for align text under time/nick */
        if ((line->length_align > 0) &&
            (window->win_chat_cursor_x == 0) &&
            (*lines_displayed > 0) &&
            /* TODO: modify arbitraty value for non aligning messages on time/nick? */
            (line->length_align < (window->win_chat_width - 5)))
        {
            if (!simulate)
                mvwprintw (window->win_chat,
                           window->win_chat_cursor_y, 
                           window->win_chat_cursor_x,
                           format_align, " ");
            window->win_chat_cursor_x += line->length_align;
        }
        
        chars_to_display = utf8_strlen (message->message + offset);

        /* too long for current line */
        if (window->win_chat_cursor_x + chars_to_display > window->win_chat_width)
        {
            num_displayed = window->win_chat_width - window->win_chat_cursor_x;
            pos_saved_char = utf8_real_pos (message->message + offset, num_displayed);
            saved_char = message->message[offset + pos_saved_char];
            message->message[offset + pos_saved_char] = '\0';
            if ((!simulate) &&
                ((count == 0) || (*lines_displayed >= num_lines - count)))
                mvwprintw (window->win_chat,
                           window->win_chat_cursor_y, 
                           window->win_chat_cursor_x,
                           "%s", message->message + offset);
            message->message[offset + pos_saved_char] = saved_char;
            offset += pos_saved_char;
        }
        else
        {
            num_displayed = chars_to_display;
            if ((!simulate) &&
                ((count == 0) || (*lines_displayed >= num_lines - count)))
                mvwprintw (window->win_chat,
                           window->win_chat_cursor_y, 
                           window->win_chat_cursor_x,
                           "%s", message->message + offset);
            if (message == end_msg)
            {
                if (end_msg)
                    end_msg->message[end_offset + 1] = saved_char_end;
                if (saved_char_end == '\0')
                {
                    message = message->next_message;
                    offset = 0;
                }
                else
                    offset = end_offset + 1;
            }
            else
            {
                message = message->next_message;
                offset = 0;
            }
        }
        
        window->win_chat_cursor_x += num_displayed;
        
        /* display new line? */
        if (!message ||
            ((window->win_chat_cursor_y <= window->win_chat_height - 1) &&
            (window->win_chat_cursor_x > (window->win_chat_width - 1))))
            gui_display_new_line (window, num_lines, count,
                                  lines_displayed, simulate);
        
        /* end of word? */
        if (!message || (message->prev_message == end_msg) ||
            ((message == end_msg) && (offset > end_offset)) ||
            (window->win_chat_cursor_y >= window->win_chat_height))
            end_of_word = 1;
    }
        
    if (end_msg)
        end_msg->message[end_offset + 1] = saved_char_end;
}

/*
 * gui_get_word_info: returns info about next word: beginning, end, length
 */

void
gui_get_word_info (t_gui_message *message, int offset,
                   t_gui_message **word_start_msg, int *word_start_offset,
                   t_gui_message **word_end_msg, int *word_end_offset,
                   int *word_length_with_spaces, int *word_length)
{
    *word_start_msg = NULL;
    *word_start_offset = 0;
    *word_end_msg = NULL;
    *word_end_offset = 0;
    *word_length_with_spaces = 0;
    *word_length = 0;
    
    /* leading spaces */
    while (message && (message->message[offset] == ' '))
    {
        (*word_length_with_spaces)++;
        gui_message_get_next_char (&message, &offset);
    }
    
    /* not only spaces? */
    if (message)
    {
        *word_start_msg = message;
        *word_start_offset = offset;
        
        /* find end of word */
        while (message && (message->message[offset]) && (message->message[offset] != ' '))
        {
            *word_end_msg = message;
            *word_end_offset = offset + utf8_char_size (message->message + offset) - 1;
            (*word_length_with_spaces) += utf8_char_size (message->message + offset);
            (*word_length) += utf8_char_size (message->message + offset);
            gui_message_get_next_char (&message, &offset);
        }
    }
}

/*
 * gui_display_line: display a line in the chat window
 *                   if count == 0, display whole line
 *                   if count > 0, display 'count' lines (beginning from the end)
 *                   if simulate == 1, nothing is displayed (for counting how
 *                                     many lines would have been lines displayed)
 *                   returns: number of lines displayed (or simulated)
 */

int
gui_display_line (t_gui_window *window, t_gui_line *line, int count, int simulate)
{
    int num_lines, x, y, offset, lines_displayed;
    t_gui_message *ptr_message, *word_start_msg, *word_end_msg;
    int word_start_offset, word_end_offset;
    int word_length_with_spaces, word_length;
    int skip_spaces;
    
    if (simulate)
    {
        x = window->win_chat_cursor_x;
        y = window->win_chat_cursor_y;
        window->win_chat_cursor_x = 0;
        window->win_chat_cursor_y = 0;
        num_lines = 0;
    }
    else
    {
        if (window->win_chat_cursor_y > window->win_chat_height - 1)
            return 0;
        x = window->win_chat_cursor_x;
        y = window->win_chat_cursor_y;
        num_lines = gui_display_line (window, line, 0, 1);
        window->win_chat_cursor_x = x;
        window->win_chat_cursor_y = y;
    }
    
    ptr_message = line->messages;
    offset = 0;
    lines_displayed = 0;
    while (ptr_message)
    {
        skip_spaces = 0;
        gui_get_word_info (ptr_message, offset,
                           &word_start_msg, &word_start_offset,
                           &word_end_msg, &word_end_offset,
                           &word_length_with_spaces, &word_length);
        
        if (word_length > 0)
        {
            /* spaces + word too long for current line */
            if ((window->win_chat_cursor_x + word_length_with_spaces > window->win_chat_width - 1)
                && (word_length < window->win_chat_width - line->length_align))
            {
                gui_display_new_line (window, num_lines, count,
                                      &lines_displayed, simulate);
                ptr_message = word_start_msg;
                offset = word_start_offset;
            }
            
            /* word is exactly width => we'll skip next leading spaces for next line */
            if (word_length == window->win_chat_width - line->length_align)
                skip_spaces = 1;
            
            /* display word */
            gui_display_word (window, line,
                              ptr_message, offset,
                              word_end_msg, word_end_offset,
                              num_lines, count, &lines_displayed, simulate);
            
            if (window->win_chat_cursor_y >= window->win_chat_height)
                ptr_message = NULL;
            else
            {
                /* move pointer after end of word */
                ptr_message = word_end_msg;
                offset = word_end_offset;
                gui_message_get_next_char (&ptr_message, &offset);
                
                /* skip leading spaces? */
                if (skip_spaces)
                {
                    while (ptr_message && (ptr_message->message[offset] == ' '))
                        gui_message_get_next_char (&ptr_message, &offset);
                }
            }
        }
        else
        {
            gui_display_new_line (window, num_lines, count,
                                  &lines_displayed, simulate);
            ptr_message = NULL;
        }
    }
    
    if (simulate)
    {
        window->win_chat_cursor_x = x;
        window->win_chat_cursor_y = y;
    }
    
    return lines_displayed;
}

/*
 * gui_calculate_line_diff: returns pointer to line & offset for a difference
 *                          with given line
 */

void
gui_calculate_line_diff (t_gui_window *window, t_gui_line **line, int *line_pos,
                         int difference)
{
    int backward, current_size;
    
    if (!line || !line_pos)
        return;
    
    backward = (difference < 0);
    
    if (!(*line))
    {
        /* if looking backward, start at last line of buffer */
        if (backward)
        {
            *line = window->buffer->last_line;
            if (!(*line))
                return;
            current_size = gui_display_line (window, *line, 0, 1);
            if (current_size == 0)
                current_size = 1;
            *line_pos = current_size - 1;
        }
        /* if looking forward, start at first line of buffer */
        else
        {
            *line = window->buffer->lines;
            if (!(*line))
                return;
            *line_pos = 0;
            current_size = gui_display_line (window, *line, 0, 1);
        }
    }
    else
        current_size = gui_display_line (window, *line, 0, 1);
    
    while ((*line) && (difference != 0))
    {
        /* looking backward */
        if (backward)
        {
            if (*line_pos > 0)
                (*line_pos)--;
            else
            {
                *line = (*line)->prev_line;
                if (*line)
                {
                    current_size = gui_display_line (window, *line, 0, 1);
                    if (current_size == 0)
                        current_size = 1;
                    *line_pos = current_size - 1;
                }
            }
            difference++;
        }
        /* looking forward */
        else
        {
            if (*line_pos < current_size - 1)
                (*line_pos)++;
            else
            {
                *line = (*line)->next_line;
                if (*line)
                {
                    current_size = gui_display_line (window, *line, 0, 1);
                    if (current_size == 0)
                        current_size = 1;
                    *line_pos = 0;
                }
            }
            difference--;
        }
    }
    
    /* first or last line reached */
    if (!(*line))
    {
        if (backward)
        {
            /* first line reached */
            *line = window->buffer->lines;
            *line_pos = 0;
        }
        else
        {
            /* last line reached => consider we'll display all until the end */
            *line_pos = 0;
        }
    }
}

/*
 * gui_draw_buffer_chat: draw chat window for a buffer
 */

void
gui_draw_buffer_chat (t_gui_buffer *buffer, int erase)
{
    t_gui_window *ptr_win;
    t_gui_line *ptr_line;
    t_irc_dcc *dcc_first, *dcc_selected, *ptr_dcc;
    char format_empty[32];
    int i, j, line_pos, count, num_bars;
    char *unit_name[] = { N_("bytes"), N_("Kb"), N_("Mb"), N_("Gb") };
    char *unit_format[] = { "%.0Lf", "%.1Lf", "%.02Lf", "%.02Lf" };
    long unit_divide[] = { 1, 1024, 1024*1024, 1024*1024,1024 };
    int num_unit;
    char format[32], date[128], *buf;
    struct tm *date_tmp;
    
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
                dcc_first = (ptr_win->dcc_first) ? (t_irc_dcc *) ptr_win->dcc_first : dcc_list;
                dcc_selected = (ptr_win->dcc_selected) ? (t_irc_dcc *) ptr_win->dcc_selected : dcc_list;
                for (ptr_dcc = dcc_first; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
                {
                    if (i >= ptr_win->win_chat_height - 1)
                        break;
                    
                    /* nickname and filename */
                    gui_window_set_color (ptr_win->win_chat,
                                          (ptr_dcc == dcc_selected) ?
                                          COLOR_DCC_SELECTED : COLOR_WIN_CHAT);
                    mvwprintw (ptr_win->win_chat, i, 0, "%s %-16s ",
                               (ptr_dcc == dcc_selected) ? "***" : "   ",
                               ptr_dcc->nick);
                    buf = weechat_convert_encoding ((local_utf8) ?
                                                    cfg_look_charset_decode_iso : cfg_look_charset_decode_utf,
                                                    (cfg_look_charset_internal && cfg_look_charset_internal[0]) ?
                                                    cfg_look_charset_internal : local_charset,
                                                    (DCC_IS_CHAT(ptr_dcc->type)) ?
                                                    _(ptr_dcc->filename) : ptr_dcc->filename);
                    wprintw (ptr_win->win_chat, "%s", buf);
                    free (buf);
                    if (DCC_IS_FILE(ptr_dcc->type))
                    {
                        if (ptr_dcc->filename_suffix > 0)
                            wprintw (ptr_win->win_chat, " (.%d)",
                                     ptr_dcc->filename_suffix);
                    }
                    
                    /* status */
                    gui_window_set_color (ptr_win->win_chat,
                                          (ptr_dcc == dcc_selected) ?
                                          COLOR_DCC_SELECTED : COLOR_WIN_CHAT);
                    mvwprintw (ptr_win->win_chat, i + 1, 0, "%s %s ",
                               (ptr_dcc == dcc_selected) ? "***" : "   ",
                               (DCC_IS_RECV(ptr_dcc->type)) ? "-->>" : "<<--");
                    gui_window_set_color (ptr_win->win_chat,
                                          COLOR_DCC_WAITING + ptr_dcc->status);
                    buf = weechat_convert_encoding ((local_utf8) ?
                                                    cfg_look_charset_decode_iso : cfg_look_charset_decode_utf,
                                                    (cfg_look_charset_internal && cfg_look_charset_internal[0]) ?
                                                    cfg_look_charset_internal : local_charset,
                                                    _(dcc_status_string[ptr_dcc->status]));
                    wprintw (ptr_win->win_chat, "%-10s", buf);
                    free (buf);
                    
                    /* other infos */
                    gui_window_set_color (ptr_win->win_chat,
                                          (ptr_dcc == dcc_selected) ?
                                          COLOR_DCC_SELECTED : COLOR_WIN_CHAT);
                    if (DCC_IS_FILE(ptr_dcc->type))
                    {
                        wprintw (ptr_win->win_chat, "  [");
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
                                 (unsigned long)(((long double)(ptr_dcc->pos)/(long double)(ptr_dcc->size))*100));
                        sprintf (format, "%s %%s / %s %%s",
                                 unit_format[num_unit],
                                 unit_format[num_unit]);
                        wprintw (ptr_win->win_chat, format,
                                 ((long double)(ptr_dcc->pos)) / ((long double)(unit_divide[num_unit])),
                                 unit_name[num_unit],
                                 ((long double)(ptr_dcc->size)) / ((long double)(unit_divide[num_unit])),
                                 unit_name[num_unit]);
                        
                        if (ptr_dcc->bytes_per_sec < 1024*1024)
                            num_unit = 1;
                        else if (ptr_dcc->bytes_per_sec < 1024*1024*1024)
                            num_unit = 2;
                        else
                            num_unit = 3;
                        sprintf (format, "  (%s %%s/s)", unit_format[num_unit]);
                        buf = weechat_convert_encoding ((local_utf8) ?
                                                        cfg_look_charset_decode_iso : cfg_look_charset_decode_utf,
                                                        (cfg_look_charset_internal && cfg_look_charset_internal[0]) ?
                                                        cfg_look_charset_internal : local_charset,
                                                        unit_name[num_unit]);
                        wprintw (ptr_win->win_chat, format,
                                 ((long double) ptr_dcc->bytes_per_sec) / ((long double)(unit_divide[num_unit])),
                                 buf);
                        free (buf);
                    }
                    else
                    {
                        date_tmp = localtime (&(ptr_dcc->start_time));
                        strftime (date, sizeof (date) - 1, "%a, %d %b %Y %H:%M:%S", date_tmp);
                        wprintw (ptr_win->win_chat, "  %s", date);
                    }
                    
                    wclrtoeol (ptr_win->win_chat);
                    
                    ptr_win->dcc_last_displayed = ptr_dcc;
                    i += 2;
                }
            }
            else
            {
                ptr_win->win_chat_cursor_x = 0;
                ptr_win->win_chat_cursor_y = 0;
                
                /* display at position of scrolling */
                if (ptr_win->start_line)
                {
                    ptr_line = ptr_win->start_line;
                    line_pos = ptr_win->start_line_pos;
                }
                else
                {
                    /* look for first line to display, sarting from last line */
                    ptr_line = NULL;
                    line_pos = 0;
                    gui_calculate_line_diff (ptr_win, &ptr_line, &line_pos,
                                             (-1) * (ptr_win->win_chat_height - 1));
                }

                if (line_pos > 0)
                {
                    /* display end of first line at top of screen */
                    gui_display_line (ptr_win, ptr_line,
                                      gui_display_line (ptr_win, ptr_line, 0, 1) -
                                      line_pos, 0);
                    ptr_line = ptr_line->next_line;
                    ptr_win->first_line_displayed = 0;
                }
                else
                    ptr_win->first_line_displayed =
                        (ptr_line == ptr_win->buffer->lines);
                
                /* display lines */
                count = 0;
                while (ptr_line && (ptr_win->win_chat_cursor_y <= ptr_win->win_chat_height - 1))
                {
                    count = gui_display_line (ptr_win, ptr_line, 0, 0);
                    ptr_line = ptr_line->next_line;
                }
                
                /* check if last line of buffer is entirely displayed and scrolling */
                /* if so, disable scroll (to remove status bar indicator) */
                if (!ptr_line && ptr_win->start_line)
                {
                    if (count == gui_display_line (ptr_win, ptr_win->buffer->last_line, 0, 1))
                    {
                        ptr_win->start_line = NULL;
                        ptr_win->start_line_pos = 0;
                    }
                }
                
                /* cursor is below end line of chat window? */
                if (ptr_win->win_chat_cursor_y > ptr_win->win_chat_height - 1)
                {
                    ptr_win->win_chat_cursor_x = 0;
                    ptr_win->win_chat_cursor_y = ptr_win->win_chat_height - 1;
                }
            }
            wnoutrefresh (ptr_win->win_chat);
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
    int i, j, x, y, column, max_length, nicks_displayed;
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
            
            if (gui_buffer_has_nicklist (buffer))
            {
                max_length = nick_get_max_length (CHANNEL(buffer));
                if ((buffer->num_displayed > 0) &&
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
                snprintf (format, 32, "%%.%ds",
                          ((cfg_look_nicklist_min_size > 0)
                           && (max_length < cfg_look_nicklist_min_size)) ?
                          cfg_look_nicklist_min_size :
                          (((cfg_look_nicklist_max_size > 0)
                           && (max_length > cfg_look_nicklist_max_size)) ?
                          cfg_look_nicklist_max_size : max_length));
                
                if (has_colors ())
                {
                    gui_window_set_color (ptr_win->win_nick, COLOR_WIN_NICK_SEP);
                    switch (cfg_look_nicklist_position)
                    {
                        case CFG_LOOK_NICKLIST_LEFT:
                            mvwvline (ptr_win->win_nick,
                                      0, ptr_win->win_nick_width - 1, ACS_VLINE,
                                      ptr_win->win_chat_height);
                            break;
                        case CFG_LOOK_NICKLIST_RIGHT:
                            mvwvline (ptr_win->win_nick,
                                      0, 0, ACS_VLINE,
                                      ptr_win->win_chat_height);
                            break;
                        case CFG_LOOK_NICKLIST_TOP:
                            mvwhline (ptr_win->win_nick,
                                      ptr_win->win_nick_height - 1, 0, ACS_HLINE,
                                      ptr_win->win_chat_width);
                            break;
                        case CFG_LOOK_NICKLIST_BOTTOM:
                            mvwhline (ptr_win->win_nick,
                                      0, 0, ACS_HLINE,
                                      ptr_win->win_chat_width);
                            break;
                    }
                }
                
                gui_window_set_color (ptr_win->win_nick, COLOR_WIN_NICK);
                x = 0;
                y = (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_BOTTOM) ? 1 : 0;
                column = 0;
                
                if ((cfg_look_nicklist_position == CFG_LOOK_NICKLIST_TOP) ||
                    (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_BOTTOM))
                    nicks_displayed = (ptr_win->win_width / (max_length + 2)) * (ptr_win->win_height - 1);
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
                                x = 1;
                                break;
                            case CFG_LOOK_NICKLIST_TOP:
                            case CFG_LOOK_NICKLIST_BOTTOM:
                                x = column;
                                break;
                        }
                        if ( ((i == 0) && (ptr_win->win_nick_start > 0))
                            || ((i == nicks_displayed - 1) && (ptr_nick->next_nick)) )
                        {
                            gui_window_set_color (ptr_win->win_nick, COLOR_WIN_NICK_MORE);
                            j = (max_length + 1) >= 4 ? 4 : max_length + 1;
                            for (x = 1; x <= j; x++)
                                mvwprintw (ptr_win->win_nick, y, x, "+");
                        }
                        else
                        {
                            if (ptr_nick->is_chanowner)
                            {
                                gui_window_set_color (ptr_win->win_nick, COLOR_WIN_NICK_CHANOWNER);
                                mvwprintw (ptr_win->win_nick, y, x, "~");
                                x++;
                            }
                            else if (ptr_nick->is_chanadmin)
                            {
                                gui_window_set_color (ptr_win->win_nick, COLOR_WIN_NICK_CHANADMIN);
                                mvwprintw (ptr_win->win_nick, y, x, "&");
                                x++;
                            }
                            else if (ptr_nick->is_op)
                            {
                                gui_window_set_color (ptr_win->win_nick, COLOR_WIN_NICK_OP);
                                mvwprintw (ptr_win->win_nick, y, x, "@");
                                x++;
                            }
                            else if (ptr_nick->is_halfop)
                            {
                                gui_window_set_color (ptr_win->win_nick, COLOR_WIN_NICK_HALFOP);
                                mvwprintw (ptr_win->win_nick, y, x, "%%");
                                x++;
                            }
                            else if (ptr_nick->has_voice)
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
                            gui_window_set_color (ptr_win->win_nick,
                                                  ((cfg_irc_away_check > 0) && (ptr_nick->is_away)) ?
                                                  COLOR_WIN_NICK_AWAY : COLOR_WIN_NICK);
                            mvwprintw (ptr_win->win_nick, y, x, format, ptr_nick->nick);
                            
                            ptr_nick = ptr_nick->next_nick;
                            
                            if (!ptr_nick)
                                break;
                        }
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
            }
            wnoutrefresh (ptr_win->win_nick);
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
    char format[32], str_nicks[32], *more;
    int i, first_mode, x, server_pos, server_total;
    int display_name, names_count;
    
    /* make gcc happy */
    (void) buffer;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (has_colors ())
            wbkgdset(ptr_win->win_status, ' ' | COLOR_PAIR (COLOR_WIN_STATUS));
        
        if (erase)
            gui_curses_window_clear (ptr_win->win_status);
        
        gui_window_set_color (ptr_win->win_status, COLOR_WIN_STATUS);
        /* wborder (ptr_win->win_status, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '); */
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
        
        /* display "<servers>" or current server */
        if (ptr_win->buffer->all_servers)
        {
            wprintw (ptr_win->win_status, "[");
            gui_window_set_color (ptr_win->win_status,
                                  COLOR_WIN_STATUS);
            wprintw (ptr_win->win_status, _("<servers>"));
            gui_window_set_color (ptr_win->win_status,
                              COLOR_WIN_STATUS_DELIMITERS);
            wprintw (ptr_win->win_status, "] ");
        }
        else if (SERVER(ptr_win->buffer) && SERVER(ptr_win->buffer)->name)
        {
            wprintw (ptr_win->win_status, "[");
            gui_window_set_color (ptr_win->win_status,
                                  COLOR_WIN_STATUS);
            wprintw (ptr_win->win_status, "%s", SERVER(ptr_win->buffer)->name);
            if (SERVER(ptr_win->buffer)->is_away)
                wprintw (ptr_win->win_status, _("(away)"));
            gui_window_set_color (ptr_win->win_status,
                              COLOR_WIN_STATUS_DELIMITERS);
            wprintw (ptr_win->win_status, "] ");
        }
        
        /* infos about current buffer */
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
                                  COLOR_WIN_STATUS_CHANNEL);
            if (SERVER(ptr_win->buffer)->is_connected)
                wprintw (ptr_win->win_status, "[%s] ",
                         SERVER(ptr_win->buffer)->name);
            else
                wprintw (ptr_win->win_status, "(%s) ",
                         SERVER(ptr_win->buffer)->name);
            if (ptr_win->buffer->all_servers)
            {
                server_get_number_buffer (SERVER(ptr_win->buffer),
                                          &server_pos,
                                          &server_total);
                gui_window_set_color (ptr_win->win_status,
                                      COLOR_WIN_STATUS_DELIMITERS);
                wprintw (ptr_win->win_status, "(");
                gui_window_set_color (ptr_win->win_status,
                                      COLOR_WIN_STATUS);
                wprintw (ptr_win->win_status, "%d", server_pos);
                gui_window_set_color (ptr_win->win_status,
                                      COLOR_WIN_STATUS_DELIMITERS);
                wprintw (ptr_win->win_status, "/");
                gui_window_set_color (ptr_win->win_status,
                                      COLOR_WIN_STATUS);
                wprintw (ptr_win->win_status, "%d", server_total);
                gui_window_set_color (ptr_win->win_status,
                                      COLOR_WIN_STATUS_DELIMITERS);
                wprintw (ptr_win->win_status, ") ");

            }
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
                                  COLOR_WIN_STATUS_CHANNEL);
            if ((!CHANNEL(ptr_win->buffer)->nicks)
                && (CHANNEL(ptr_win->buffer)->type != CHAT_PRIVATE))
                wprintw (ptr_win->win_status, "(%s)",
                         CHANNEL(ptr_win->buffer)->name);
            else
                wprintw (ptr_win->win_status, "%s",
                         CHANNEL(ptr_win->buffer)->name);
            if (ptr_win->buffer == CHANNEL(ptr_win->buffer)->buffer)
            {
                /* display channel modes */
                if (CHANNEL(ptr_win->buffer)->type == CHAT_CHANNEL)
                {
                    gui_window_set_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS_DELIMITERS);
                    wprintw (ptr_win->win_status, "(");
                    gui_window_set_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS);
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
                    gui_window_set_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS_DELIMITERS);
                    wprintw (ptr_win->win_status, ")");
                    gui_window_set_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS);
                }
                
                /* display DCC if private is DCC CHAT */
                if ((CHANNEL(ptr_win->buffer)->type == CHAT_PRIVATE)
                    && (CHANNEL(ptr_win->buffer)->dcc_chat))
                {
                    gui_window_set_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS_DELIMITERS);
                    wprintw (ptr_win->win_status, "(");
                    gui_window_set_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS_CHANNEL);
                    wprintw (ptr_win->win_status, "DCC");
                    gui_window_set_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS_DELIMITERS);
                    wprintw (ptr_win->win_status, ")");
                    gui_window_set_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS);
                }
            }
            wprintw (ptr_win->win_status, " ");
        }
        if (!SERVER(ptr_win->buffer))
        {
            gui_window_set_color (ptr_win->win_status, COLOR_WIN_STATUS);
            wprintw (ptr_win->win_status, "%d",
                     ptr_win->buffer->number);
            gui_window_set_color (ptr_win->win_status, COLOR_WIN_STATUS_DELIMITERS);
            wprintw (ptr_win->win_status, ":");
            gui_window_set_color (ptr_win->win_status, COLOR_WIN_STATUS_CHANNEL);
            if (ptr_win->buffer->dcc)
                wprintw (ptr_win->win_status, "<DCC> ");
            else
                wprintw (ptr_win->win_status, _("[not connected] "));
        }
        
        /* display list of other active windows (if any) with numbers */
        if (hotlist)
        {
            gui_window_set_color (ptr_win->win_status,
                                  COLOR_WIN_STATUS_DELIMITERS);
            wprintw (ptr_win->win_status, "[");
            gui_window_set_color (ptr_win->win_status, COLOR_WIN_STATUS);
            wprintw (ptr_win->win_status, _("Act: "));
            
            names_count = 0;
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                switch (ptr_hotlist->priority)
                {
                    case HOTLIST_LOW:
                        gui_window_set_color (ptr_win->win_status,
                                              COLOR_WIN_STATUS_DATA_OTHER);
                        display_name = ((cfg_look_hotlist_names_level & 1) != 0);
                        break;
                    case HOTLIST_MSG:
                        gui_window_set_color (ptr_win->win_status,
                                              COLOR_WIN_STATUS_DATA_MSG);
                        display_name = ((cfg_look_hotlist_names_level & 2) != 0);
                        break;
                    case HOTLIST_PRIVATE:
                        gui_window_set_color (ptr_win->win_status,
                                              COLOR_WIN_STATUS_DATA_PRIVATE);
                        display_name = ((cfg_look_hotlist_names_level & 4) != 0);
                        break;
                    case HOTLIST_HIGHLIGHT:
                        gui_window_set_color (ptr_win->win_status,
                                              COLOR_WIN_STATUS_DATA_HIGHLIGHT);
                        display_name = ((cfg_look_hotlist_names_level & 8) != 0);
                        break;
                    default:
                        display_name = 0;
                        break;
                }
                if (ptr_hotlist->buffer->dcc)
                {
                    wprintw (ptr_win->win_status, "%d",
                             ptr_hotlist->buffer->number);
                    gui_window_set_color (ptr_win->win_status,
                                              COLOR_WIN_STATUS_DELIMITERS);
                    wprintw (ptr_win->win_status, ":");
                    gui_window_set_color (ptr_win->win_status,
                                          COLOR_WIN_STATUS);
                    wprintw (ptr_win->win_status, "DCC");
                }
                else
                {
                    wprintw (ptr_win->win_status, "%d",
                             ptr_hotlist->buffer->number);
                
                    if (display_name && (cfg_look_hotlist_names_count != 0)
                        && (names_count < cfg_look_hotlist_names_count))
                    {
                        names_count++;
                        
                        gui_window_set_color (ptr_win->win_status,
                                              COLOR_WIN_STATUS_DELIMITERS);
                        wprintw (ptr_win->win_status, ":");
                        
                        gui_window_set_color (ptr_win->win_status,
                                              COLOR_WIN_STATUS);
                        if (cfg_look_hotlist_names_length == 0)
                            snprintf (format, sizeof (format) - 1, "%%s");
                        else
                            snprintf (format, sizeof (format) - 1, "%%.%ds", cfg_look_hotlist_names_length);
                        if (BUFFER_IS_SERVER(ptr_hotlist->buffer))
                            wprintw (ptr_win->win_status, format, SERVER(ptr_hotlist->buffer)->name);
                        else if (BUFFER_IS_CHANNEL(ptr_hotlist->buffer)
                                 || BUFFER_IS_PRIVATE(ptr_hotlist->buffer))
                            wprintw (ptr_win->win_status, format, CHANNEL(ptr_hotlist->buffer)->name);
                    }
                }
                
                if (ptr_hotlist->next_hotlist)
                    wprintw (ptr_win->win_status, ",");
            }
            gui_window_set_color (ptr_win->win_status,
                                  COLOR_WIN_STATUS_DELIMITERS);
            wprintw (ptr_win->win_status, "] ");
        }
        
        /* display lag */
        if (SERVER(ptr_win->buffer))
        {
            if (SERVER(ptr_win->buffer)->lag / 1000 >= cfg_irc_lag_min_show)
            {
                gui_window_set_color (ptr_win->win_status,
                                      COLOR_WIN_STATUS_DELIMITERS);
                wprintw (ptr_win->win_status, "[");
                gui_window_set_color (ptr_win->win_status, COLOR_WIN_STATUS);
                wprintw (ptr_win->win_status, _("Lag: %.1f"),
                         ((float)(SERVER(ptr_win->buffer)->lag)) / 1000);
                gui_window_set_color (ptr_win->win_status,
                                      COLOR_WIN_STATUS_DELIMITERS);
                wprintw (ptr_win->win_status, "]");
            }
        }
        
        /* display "-MORE-" (if last line is not displayed) & nicks count */
        if (gui_buffer_has_nicklist (ptr_win->buffer))
        {
            snprintf (str_nicks, sizeof (str_nicks) - 1, "%d", CHANNEL(ptr_win->buffer)->nicks_count);
            x = ptr_win->win_width - strlen (str_nicks) - 4;
        }
        else
            x = ptr_win->win_width - 2;
        more = strdup (_("-MORE-"));
        x -= strlen (more) - 1;
        if (x < 0)
            x = 0;
        gui_window_set_color (ptr_win->win_status, COLOR_WIN_STATUS_MORE);
        if (ptr_win->start_line)
            mvwprintw (ptr_win->win_status, 0, x, "%s", more);
        else
        {
            snprintf (format, sizeof (format) - 1, "%%-%ds", (int)(strlen (more)));
            mvwprintw (ptr_win->win_status, 0, x, format, " ");
        }
        if (gui_buffer_has_nicklist (ptr_win->buffer))
        {
            gui_window_set_color (ptr_win->win_status,
                                  COLOR_WIN_STATUS_DELIMITERS);
            wprintw (ptr_win->win_status, " [");
            gui_window_set_color (ptr_win->win_status, COLOR_WIN_STATUS);
            wprintw (ptr_win->win_status, "%s", str_nicks);
            gui_window_set_color (ptr_win->win_status,
                                  COLOR_WIN_STATUS_DELIMITERS);
            wprintw (ptr_win->win_status, "]");
        }
        free (more);
        
        wnoutrefresh (ptr_win->win_status);
        refresh ();
    }
}

/*
 * gui_draw_buffer_infobar_time: draw time in infobar window
 */

void
gui_draw_buffer_infobar_time (t_gui_buffer *buffer)
{
    t_gui_window *ptr_win;
    time_t time_seconds;
    struct tm *local_time;
    
    /* make gcc happy */
    (void) buffer;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {        
        time_seconds = time (NULL);
        local_time = localtime (&time_seconds);
        if (local_time)
        {
            gui_window_set_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR);
            mvwprintw (ptr_win->win_infobar,
                       0, 1,
                       "%02d:%02d",
                       local_time->tm_hour, local_time->tm_min);
            if (cfg_look_infobar_seconds)
                wprintw (ptr_win->win_infobar,
                         ":%02d",
                         local_time->tm_sec);
        }
        wnoutrefresh (ptr_win->win_infobar);
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
    char text_time[1024 + 1];
    
    /* make gcc happy */
    (void) buffer;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (has_colors ())
            wbkgdset(ptr_win->win_infobar, ' ' | COLOR_PAIR (COLOR_WIN_INFOBAR));
        
        if (erase)
            gui_curses_window_clear (ptr_win->win_infobar);

        gui_window_set_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR);
        /* wborder (ptr_win->win_infobar, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '); */
        wmove (ptr_win->win_infobar, 0, 0);
        
        time_seconds = time (NULL);
        local_time = localtime (&time_seconds);
        if (local_time)
        {
            strftime (text_time, 1024, cfg_look_infobar_timestamp, local_time);
            gui_window_set_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR_DELIMITERS);
            wprintw (ptr_win->win_infobar, "[");
            gui_window_set_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR);
            wprintw (ptr_win->win_infobar,
                     "%02d:%02d",
                     local_time->tm_hour, local_time->tm_min);
            if (cfg_look_infobar_seconds)
                wprintw (ptr_win->win_infobar,
                         ":%02d",
                         local_time->tm_sec);
            gui_window_set_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR_DELIMITERS);
            wprintw (ptr_win->win_infobar, "]");
            gui_window_set_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR);
            wprintw (ptr_win->win_infobar,
                     " %s", text_time);
        }
        if (gui_infobar)
        {
            gui_window_set_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR_DELIMITERS);
            wprintw (ptr_win->win_infobar, " | ");
            gui_window_set_color (ptr_win->win_infobar, gui_infobar->color);
            wprintw (ptr_win->win_infobar, "%s", gui_infobar->text);
        }
        
        wnoutrefresh (ptr_win->win_infobar);
        refresh ();
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
    t_irc_dcc *dcc_selected;
    
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
                wnoutrefresh (ptr_win->win_input);
            }
            
            if (buffer->dcc)
            {
                dcc_selected = (ptr_win->dcc_selected) ? (t_irc_dcc *) ptr_win->dcc_selected : dcc_list;
                wmove (ptr_win->win_input, 0, 0);
                if (dcc_selected)
                {
                    switch (dcc_selected->status)
                    {
                        case DCC_WAITING:
                            if (DCC_IS_RECV(dcc_selected->type))
                                wprintw (ptr_win->win_input, _("  [A] Accept"));
                            wprintw (ptr_win->win_input, _("  [C] Cancel"));
                            break;
                        case DCC_CONNECTING:
                        case DCC_ACTIVE:
                            wprintw (ptr_win->win_input, _("  [C] Cancel"));
                            break;
                        case DCC_DONE:
                        case DCC_FAILED:
                        case DCC_ABORTED:
                            wprintw (ptr_win->win_input, _("  [R] Remove"));
                            break;
                    }
                }
                wprintw (ptr_win->win_input, _("  [P] Purge old DCC"));
                wprintw (ptr_win->win_input, _("  [Q] Close DCC view"));
                wclrtoeol (ptr_win->win_input);
                ptr_win->win_input_x = 0;
                if (ptr_win == gui_current_window)
                    move (ptr_win->win_y + ptr_win->win_height - 1,
                          ptr_win->win_x);
            }
            else if (buffer->has_input)
            {
                if (buffer->input_buffer_length == 0)
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
                    gui_window_set_color (ptr_win->win_input, COLOR_WIN_INPUT_CHANNEL);
                    mvwprintw (ptr_win->win_input, 0, 0, "%s ", CHANNEL(buffer)->name);
                    gui_window_set_color (ptr_win->win_input, COLOR_WIN_INPUT_NICK);
                    wprintw (ptr_win->win_input, "%s", SERVER(buffer)->nick);
                    gui_window_set_color (ptr_win->win_input, COLOR_WIN_INPUT_DELIMITERS);
                    wprintw (ptr_win->win_input, "> ");
                    gui_window_set_color (ptr_win->win_input, COLOR_WIN_INPUT);
                    snprintf (format, 32, "%%-%ds", input_width);
                    if (ptr_win == gui_current_window)
                        wprintw (ptr_win->win_input, format,
                                 utf8_add_offset (buffer->input_buffer,
                                                  buffer->input_buffer_1st_display));
                    else
                        wprintw (ptr_win->win_input, format,
                                 "");
                    wclrtoeol (ptr_win->win_input);
                    ptr_win->win_input_x = utf8_strlen (CHANNEL(buffer)->name) +
                        utf8_strlen (SERVER(buffer)->nick) + 3 +
                        (buffer->input_buffer_pos - buffer->input_buffer_1st_display);
                    if (ptr_win == gui_current_window)
                        move (ptr_win->win_y + ptr_win->win_height - 1,
                              ptr_win->win_x + ptr_win->win_input_x);
                }
                else
                {
                    gui_window_set_color (ptr_win->win_input, COLOR_WIN_INPUT_NICK);
                    if (SERVER(buffer) && (SERVER(buffer)->is_connected))
                        ptr_nickname = SERVER(buffer)->nick;
                    else
                        ptr_nickname = cfg_look_no_nickname;
                    mvwprintw (ptr_win->win_input, 0, 0, "%s", ptr_nickname);
                    gui_window_set_color (ptr_win->win_input, COLOR_WIN_INPUT_DELIMITERS);
                    wprintw (ptr_win->win_input, "> ");
                    gui_window_set_color (ptr_win->win_input, COLOR_WIN_INPUT);
                    snprintf (format, 32, "%%-%ds", input_width);
                    if (ptr_win == gui_current_window)
                        wprintw (ptr_win->win_input, format,
                                 utf8_add_offset (buffer->input_buffer,
                                                  buffer->input_buffer_1st_display));
                    else
                        wprintw (ptr_win->win_input, format,
                                 "");
                    wclrtoeol (ptr_win->win_input);
                    ptr_win->win_input_x = utf8_strlen (ptr_nickname) + 2 +
                        (buffer->input_buffer_pos - buffer->input_buffer_1st_display);
                    if (ptr_win == gui_current_window)
                        move (ptr_win->win_y + ptr_win->win_height - 1,
                              ptr_win->win_x + ptr_win->win_input_x);
                }
            }
            
            doupdate ();
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
    window->win_nick_start = 0;
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
    
    window->start_line = NULL;
    window->start_line_pos = 0;
    
    buffer->num_displayed++;
    
    hotlist_remove_buffer (buffer);
}

/*
 * gui_get_dcc_buffer: get pointer to DCC buffer (DCC buffer created if not existing)
 */

t_gui_buffer *
gui_get_dcc_buffer (t_gui_window *window)
{
    t_gui_buffer *ptr_buffer;
    
    /* check if dcc buffer exists */
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->dcc)
            break;
    }
    if (ptr_buffer)
        return ptr_buffer;
    else
        return gui_buffer_new (window, NULL, NULL, 1, 0);
}

/*
 * gui_window_page_up: display previous page on buffer
 */

void
gui_window_page_up (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (!window->first_line_displayed)
    {
        gui_calculate_line_diff (window, &window->start_line,
                                 &window->start_line_pos,
                                 (window->start_line) ?
                                 (-1) * (window->win_chat_height - 1) :
                                 (-1) * ((window->win_chat_height - 1) * 2));
        gui_draw_buffer_chat (window->buffer, 0);
        gui_draw_buffer_status (window->buffer, 0);
    }
}

/*
 * gui_window_page_down: display next page on buffer
 */

void
gui_window_page_down (t_gui_window *window)
{
    t_gui_line *ptr_line;
    int line_pos;
    
    if (!gui_ok)
        return;
    
    if (window->start_line)
    {
        gui_calculate_line_diff (window, &window->start_line,
                                 &window->start_line_pos,
                                 window->win_chat_height - 1);
        
        /* check if we can display all */
        ptr_line = window->start_line;
        line_pos = window->start_line_pos;
        gui_calculate_line_diff (window, &ptr_line,
                                 &line_pos,
                                 window->win_chat_height - 1);
        if (!ptr_line)
        {
            window->start_line = NULL;
            window->start_line_pos = 0;
        }
        
        gui_draw_buffer_chat (window->buffer, 0);
        gui_draw_buffer_status (window->buffer, 0);
    }
}

/*
 * gui_window_nick_beginning: go to beginning of nicklist
 */

void
gui_window_nick_beginning (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (gui_buffer_has_nicklist (window->buffer))
    {
        if (window->win_nick_start > 0)
        {
            window->win_nick_start = 0;
            gui_draw_buffer_nick (window->buffer, 1);
        }
    }
}

/*
 * gui_window_nick_end: go to the end of nicklist
 */

void
gui_window_nick_end (t_gui_window *window)
{
    int new_start;
    
    if (!gui_ok)
        return;
    
    if (gui_buffer_has_nicklist (window->buffer))
    {
        new_start =
            CHANNEL(window->buffer)->nicks_count - window->win_nick_height;
        if (new_start < 0)
            new_start = 0;
        else if (new_start >= 1)
            new_start++;
        
        if (new_start != window->win_nick_start)
        {
            window->win_nick_start = new_start;
            gui_draw_buffer_nick (window->buffer, 1);
        }
    }
}

/*
 * gui_window_nick_page_up: scroll one page up in nicklist
 */

void
gui_window_nick_page_up (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (gui_buffer_has_nicklist (window->buffer))
    {
        if (window->win_nick_start > 0)
        {
            window->win_nick_start -= (window->win_nick_height - 1);
            if (window->win_nick_start <= 1)
                window->win_nick_start = 0;
            gui_draw_buffer_nick (window->buffer, 1);
        }
    }
}

/*
 * gui_window_nick_page_down: scroll one page down in nicklist
 */

void
gui_window_nick_page_down (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (gui_buffer_has_nicklist (window->buffer))
    {
        if ((CHANNEL(window->buffer)->nicks_count > window->win_nick_height)
            && (window->win_nick_start + window->win_nick_height - 1
                < CHANNEL(window->buffer)->nicks_count))
        {
            if (window->win_nick_start == 0)
                window->win_nick_start += (window->win_nick_height - 1);
            else
                window->win_nick_start += (window->win_nick_height - 2);
            gui_draw_buffer_nick (window->buffer, 1);
        }
    }
}

/*
 * gui_window_init_subwindows: init subviews for a WeeChat window
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
        new_window->buffer->num_displayed++;
        
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
        new_window->buffer->num_displayed++;
        
        gui_switch_to_buffer (window, window->buffer);
        
        gui_current_window = new_window;
        gui_switch_to_buffer (gui_current_window, gui_current_window->buffer);
        gui_redraw_buffer (gui_current_window->buffer);
        
        /* create & draw separator */
        gui_draw_window_separator (gui_current_window);
    }
}

/*
 * gui_window_merge_down: merge window, direction down
 */

int
gui_window_merge_down (t_gui_window *window)
{
    t_gui_window *ptr_win;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win != window)
        {
            if ((ptr_win->win_y == window->win_y + window->win_height)
                && (ptr_win->win_x == window->win_x)
                && (ptr_win->win_width == window->win_width))
            {
                window->win_height += ptr_win->win_height;
                gui_window_free (ptr_win);
                gui_switch_to_buffer (window, window->buffer);
                gui_redraw_buffer (window->buffer);
                return 0;
            }
        }
    }
    
    /* no window found below current window */
    return -1;
}

/*
 * gui_window_merge_up: merge window, direction up
 */

int
gui_window_merge_up (t_gui_window *window)
{
    t_gui_window *ptr_win;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win != window)
        {
            if ((ptr_win->win_y + ptr_win->win_height == window->win_y)
                && (ptr_win->win_x == window->win_x)
                && (ptr_win->win_width == window->win_width))
            {
                window->win_height += ptr_win->win_height;
                window->win_y -= ptr_win->win_height;
                gui_window_free (ptr_win);
                gui_switch_to_buffer (window, window->buffer);
                gui_redraw_buffer (window->buffer);
                return 0;
            }
        }
    }
    
    /* no window found above current window */
    return -1;
}

/*
 * gui_window_merge_left: merge window, direction left
 */

int
gui_window_merge_left (t_gui_window *window)
{
    t_gui_window *ptr_win;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win != window)
        {
            if ((ptr_win->win_x + ptr_win->win_width + 1 == window->win_x)
                && (ptr_win->win_y == window->win_y)
                && (ptr_win->win_height == window->win_height))
            {
                window->win_width += ptr_win->win_width + 1;
                window->win_x -= ptr_win->win_width + 1;
                gui_window_free (ptr_win);
                gui_switch_to_buffer (window, window->buffer);
                gui_redraw_buffer (window->buffer);
                return 0;
            }
        }
    }
    
    /* no window found on the left of current window */
    return -1;
}

/*
 * gui_window_merge_right: merge window, direction right
 */

int
gui_window_merge_right (t_gui_window *window)
{
    t_gui_window *ptr_win;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win != window)
        {
            if ((ptr_win->win_x == window->win_x + window->win_width + 1)
                && (ptr_win->win_y == window->win_y)
                && (ptr_win->win_height == window->win_height))
            {
                window->win_width += ptr_win->win_width + 1;
                gui_window_free (ptr_win);
                gui_switch_to_buffer (window, window->buffer);
                gui_redraw_buffer (window->buffer);
                return 0;
            }
        }
    }
    
    /* no window found on the right of current window */
    return -1;
}

/*
 * gui_window_merge: merge a window, direction auto
 */

void
gui_window_merge_auto (t_gui_window *window)
{
    if (gui_window_merge_down (window) == 0)
        return;
    if (gui_window_merge_up (window) == 0)
        return;
    if (gui_window_merge_left (window) == 0)
        return;
    if (gui_window_merge_right (window) == 0)
        return;
}

/*
 * gui_window_merge_all: merge all windows
 */

void
gui_window_merge_all (t_gui_window *window)
{
    while (gui_windows->next_window)
    {
        gui_window_free ((gui_windows == window) ? gui_windows->next_window : gui_windows);
    }
    window->win_x = 0;
    window->win_y = 0;
    window->win_width = COLS;
    window->win_height = LINES;
    gui_switch_to_buffer (window, window->buffer);
    gui_redraw_buffer (window->buffer);
}

/*
 * gui_refresh_screen: called when term size is modified
 */

void
gui_refresh_screen ()
{
    t_gui_window *ptr_win, *old_current_window;
    int old_width, old_height;
    int new_width, new_height;
    int merge_all_windows;
    
    getmaxyx (stdscr, old_height, old_width);
    
    endwin ();
    refresh ();
    
    getmaxyx (stdscr, new_height, new_width);
    
    old_current_window = gui_current_window;
    
    gui_ok = ((new_width > 5) && (new_height > 5));
    
    merge_all_windows = 0;
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        ptr_win->dcc_first = NULL;
        ptr_win->dcc_selected = NULL;
        
        if (!merge_all_windows)
        {
            if ((ptr_win->win_x > new_width - 5)
                || (ptr_win->win_y > new_height - 5))
                merge_all_windows = 1;
            else
            {
                if (ptr_win->win_x + ptr_win->win_width == old_width)
                    ptr_win->win_width = new_width - ptr_win->win_x;
                if (ptr_win->win_y + ptr_win->win_height == old_height)
                    ptr_win->win_height = new_height - ptr_win->win_y;
                if ((ptr_win->win_width < 5) || (ptr_win->win_height < 5))
                    merge_all_windows = 1;
            }
        }
    }
    
    if (merge_all_windows)
        gui_window_merge_all (gui_current_window);
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        gui_switch_to_buffer (ptr_win, ptr_win->buffer);
        gui_redraw_buffer (ptr_win->buffer);
        gui_draw_window_separator (ptr_win);
    }
    
    gui_current_window = old_current_window;
    gui_redraw_buffer (gui_current_window->buffer);
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
    int i;
    
    if (has_colors ())
    {
        start_color ();
        use_default_colors ();
        
        init_pair (COLOR_WIN_TITLE,
            cfg_col_title, cfg_col_title_bg);
        init_pair (COLOR_WIN_CHAT,
            cfg_col_chat, cfg_col_chat_bg);
        init_pair (COLOR_WIN_CHAT_TIME,
            cfg_col_chat_time, cfg_col_chat_bg);
        init_pair (COLOR_WIN_CHAT_TIME_SEP,
            cfg_col_chat_time_sep, cfg_col_chat_bg);
        init_pair (COLOR_WIN_CHAT_PREFIX1,
            cfg_col_chat_prefix1, cfg_col_chat_bg);
        init_pair (COLOR_WIN_CHAT_PREFIX2,
            cfg_col_chat_prefix2, cfg_col_chat_bg);
        init_pair (COLOR_WIN_CHAT_SERVER,
            cfg_col_chat_server, cfg_col_chat_bg);
        init_pair (COLOR_WIN_CHAT_JOIN,
            cfg_col_chat_join, cfg_col_chat_bg);
        init_pair (COLOR_WIN_CHAT_PART,
            cfg_col_chat_part, cfg_col_chat_bg);
        init_pair (COLOR_WIN_CHAT_NICK,
            cfg_col_chat_nick, cfg_col_chat_bg);
        init_pair (COLOR_WIN_CHAT_HOST,
            cfg_col_chat_host, cfg_col_chat_bg);
        init_pair (COLOR_WIN_CHAT_CHANNEL,
            cfg_col_chat_channel, cfg_col_chat_bg);
        init_pair (COLOR_WIN_CHAT_DARK,
            cfg_col_chat_dark, cfg_col_chat_bg);
        init_pair (COLOR_WIN_CHAT_HIGHLIGHT,
            cfg_col_chat_highlight, cfg_col_chat_bg);
        init_pair (COLOR_WIN_STATUS,
            cfg_col_status, cfg_col_status_bg);
        init_pair (COLOR_WIN_STATUS_DELIMITERS,
            cfg_col_status_delimiters, cfg_col_status_bg);
        init_pair (COLOR_WIN_STATUS_CHANNEL,
            cfg_col_status_channel, cfg_col_status_bg);
        init_pair (COLOR_WIN_STATUS_DATA_MSG,
            cfg_col_status_data_msg, cfg_col_status_bg);
        init_pair (COLOR_WIN_STATUS_DATA_PRIVATE,
            cfg_col_status_data_private, cfg_col_status_bg);
        init_pair (COLOR_WIN_STATUS_DATA_HIGHLIGHT,
            cfg_col_status_data_highlight, cfg_col_status_bg);
        init_pair (COLOR_WIN_STATUS_DATA_OTHER,
            cfg_col_status_data_other, cfg_col_status_bg);
        init_pair (COLOR_WIN_STATUS_MORE,
            cfg_col_status_more, cfg_col_status_bg);
        init_pair (COLOR_WIN_INFOBAR,
            cfg_col_infobar, cfg_col_infobar_bg);
        init_pair (COLOR_WIN_INFOBAR_DELIMITERS,
            cfg_col_infobar_delimiters, cfg_col_infobar_bg);
        init_pair (COLOR_WIN_INFOBAR_HIGHLIGHT,
            cfg_col_infobar_highlight, cfg_col_infobar_bg);
        init_pair (COLOR_WIN_INPUT,
            cfg_col_input, cfg_col_input_bg);
        init_pair (COLOR_WIN_INPUT_CHANNEL,
            cfg_col_input_channel, cfg_col_input_bg);
        init_pair (COLOR_WIN_INPUT_NICK,
            cfg_col_input_nick, cfg_col_input_bg);
        init_pair (COLOR_WIN_INPUT_DELIMITERS,
            cfg_col_input_delimiters, cfg_col_input_bg);
        init_pair (COLOR_WIN_NICK,
            cfg_col_nick, cfg_col_nick_bg);
        init_pair (COLOR_WIN_NICK_AWAY,
            cfg_col_nick_away, cfg_col_nick_bg);
        init_pair (COLOR_WIN_NICK_CHANOWNER,
            cfg_col_nick_chanowner, cfg_col_nick_bg);
        init_pair (COLOR_WIN_NICK_CHANADMIN,
            cfg_col_nick_chanadmin, cfg_col_nick_bg);
        init_pair (COLOR_WIN_NICK_OP,
            cfg_col_nick_op, cfg_col_nick_bg);
        init_pair (COLOR_WIN_NICK_HALFOP,
            cfg_col_nick_halfop, cfg_col_nick_bg);
        init_pair (COLOR_WIN_NICK_VOICE,
            cfg_col_nick_voice, cfg_col_nick_bg);
        init_pair (COLOR_WIN_NICK_MORE,
            cfg_col_nick_more, cfg_col_nick_bg);
        init_pair (COLOR_WIN_NICK_SEP,
            cfg_col_nick_sep, cfg_col_nick_bg);
        init_pair (COLOR_WIN_NICK_SELF,
            cfg_col_nick_self, cfg_col_nick_bg);
        init_pair (COLOR_WIN_NICK_PRIVATE,
            cfg_col_nick_private, cfg_col_nick_bg);
        
        for (i = 0; i < COLOR_WIN_NICK_NUMBER; i++)
        {
            init_pair (COLOR_WIN_NICK_FIRST + i, cfg_col_nick_colors[i], cfg_col_chat_bg);
            color_attr[COLOR_WIN_NICK_FIRST + i - 1] = (cfg_col_nick_colors[i] >= 0) ? cfg_col_nick_colors[i] & A_BOLD : 0;
        }
        
        init_pair (COLOR_DCC_SELECTED,
            cfg_col_dcc_selected, cfg_col_chat_bg);
        init_pair (COLOR_DCC_WAITING,
            cfg_col_dcc_waiting, cfg_col_chat_bg);
        init_pair (COLOR_DCC_CONNECTING,
            cfg_col_dcc_connecting, cfg_col_chat_bg);
        init_pair (COLOR_DCC_ACTIVE,
            cfg_col_dcc_active, cfg_col_chat_bg);
        init_pair (COLOR_DCC_DONE,
            cfg_col_dcc_done, cfg_col_chat_bg);
        init_pair (COLOR_DCC_FAILED,
            cfg_col_dcc_failed, cfg_col_chat_bg);
        init_pair (COLOR_DCC_ABORTED,
            cfg_col_dcc_aborted, cfg_col_chat_bg);
            
        color_attr[COLOR_WIN_TITLE - 1] = (cfg_col_title >= 0) ? cfg_col_title & A_BOLD : 0;
        color_attr[COLOR_WIN_CHAT - 1] = (cfg_col_chat >= 0) ? cfg_col_chat & A_BOLD : 0;
        color_attr[COLOR_WIN_CHAT_TIME - 1] = (cfg_col_chat_time >= 0) ? cfg_col_chat_time & A_BOLD : 0;
        color_attr[COLOR_WIN_CHAT_TIME_SEP - 1] = (cfg_col_chat_time_sep >= 0) ? cfg_col_chat_time_sep & A_BOLD : 0;
        color_attr[COLOR_WIN_CHAT_DARK - 1] = (cfg_col_chat_dark >= 0) ? cfg_col_chat_dark & A_BOLD : 0;
        color_attr[COLOR_WIN_CHAT_PREFIX1 - 1] = (cfg_col_chat_prefix1 >= 0) ? cfg_col_chat_prefix1 & A_BOLD : 0;
        color_attr[COLOR_WIN_CHAT_PREFIX2 - 1] = (cfg_col_chat_prefix2 >= 0) ? cfg_col_chat_prefix2 & A_BOLD : 0;
        color_attr[COLOR_WIN_CHAT_SERVER - 1] = (cfg_col_chat_server >= 0) ? cfg_col_chat_server & A_BOLD : 0;
        color_attr[COLOR_WIN_CHAT_JOIN - 1] = (cfg_col_chat_join >= 0) ? cfg_col_chat_join & A_BOLD : 0;
        color_attr[COLOR_WIN_CHAT_PART - 1] = (cfg_col_chat_part >= 0) ? cfg_col_chat_part & A_BOLD : 0;
        color_attr[COLOR_WIN_CHAT_NICK - 1] = (cfg_col_chat_nick >= 0) ? cfg_col_chat_nick & A_BOLD : 0;
        color_attr[COLOR_WIN_CHAT_HOST - 1] = (cfg_col_chat_host >= 0) ? cfg_col_chat_host & A_BOLD : 0;
        color_attr[COLOR_WIN_CHAT_CHANNEL - 1] = (cfg_col_chat_channel >= 0) ? cfg_col_chat_channel & A_BOLD : 0;
        color_attr[COLOR_WIN_CHAT_DARK - 1] = (cfg_col_chat_dark >= 0) ? cfg_col_chat_dark & A_BOLD : 0;
        color_attr[COLOR_WIN_CHAT_HIGHLIGHT - 1] = (cfg_col_chat_highlight >= 0) ? cfg_col_chat_highlight & A_BOLD : 0;
        color_attr[COLOR_WIN_STATUS - 1] = (cfg_col_status >= 0) ? cfg_col_status & A_BOLD : 0;
        color_attr[COLOR_WIN_STATUS_DELIMITERS - 1] = (cfg_col_status_delimiters >= 0) ? cfg_col_status_delimiters & A_BOLD : 0;
        color_attr[COLOR_WIN_STATUS_CHANNEL - 1] = (cfg_col_status_channel >= 0) ? cfg_col_status_channel & A_BOLD : 0;
        color_attr[COLOR_WIN_STATUS_DATA_MSG - 1] = (cfg_col_status_data_msg >= 0) ? cfg_col_status_data_msg & A_BOLD : 0;
        color_attr[COLOR_WIN_STATUS_DATA_PRIVATE - 1] = (cfg_col_status_data_private >= 0) ? cfg_col_status_data_private & A_BOLD : 0;
        color_attr[COLOR_WIN_STATUS_DATA_HIGHLIGHT - 1] = (cfg_col_status_data_highlight >= 0) ? cfg_col_status_data_highlight & A_BOLD : 0;
        color_attr[COLOR_WIN_STATUS_DATA_OTHER - 1] = (cfg_col_status_data_other >= 0) ? cfg_col_status_data_other & A_BOLD : 0;
        color_attr[COLOR_WIN_STATUS_MORE - 1] = (cfg_col_status_more >= 0) ? cfg_col_status_more & A_BOLD : 0;
        color_attr[COLOR_WIN_INFOBAR - 1] = (cfg_col_infobar >= 0) ? cfg_col_infobar & A_BOLD : 0;
        color_attr[COLOR_WIN_INFOBAR_DELIMITERS - 1] = (cfg_col_infobar_delimiters >= 0) ? cfg_col_infobar_delimiters & A_BOLD : 0;
        color_attr[COLOR_WIN_INFOBAR_HIGHLIGHT - 1] = (cfg_col_infobar_highlight >= 0) ? cfg_col_infobar_highlight & A_BOLD : 0;
        color_attr[COLOR_WIN_INPUT - 1] = (cfg_col_input >= 0) ? cfg_col_input & A_BOLD : 0;
        color_attr[COLOR_WIN_INPUT_CHANNEL - 1] = (cfg_col_input_channel >= 0) ? cfg_col_input_channel & A_BOLD : 0;
        color_attr[COLOR_WIN_INPUT_NICK - 1] = (cfg_col_input_nick >= 0) ? cfg_col_input_nick & A_BOLD : 0;
        color_attr[COLOR_WIN_INPUT_DELIMITERS - 1] = (cfg_col_input_delimiters >= 0) ? cfg_col_input_delimiters & A_BOLD : 0;
        color_attr[COLOR_WIN_NICK - 1] = (cfg_col_nick >= 0) ? cfg_col_nick & A_BOLD : 0;
        color_attr[COLOR_WIN_NICK_AWAY - 1] = (cfg_col_nick_away >= 0) ? cfg_col_nick_away & A_BOLD : 0;
        color_attr[COLOR_WIN_NICK_CHANOWNER - 1] = (cfg_col_nick_chanowner >= 0) ? cfg_col_nick_chanowner & A_BOLD : 0;
        color_attr[COLOR_WIN_NICK_CHANADMIN - 1] = (cfg_col_nick_chanadmin >= 0) ? cfg_col_nick_chanadmin & A_BOLD : 0;
        color_attr[COLOR_WIN_NICK_OP - 1] = (cfg_col_nick_op >= 0) ? cfg_col_nick_op & A_BOLD : 0;
        color_attr[COLOR_WIN_NICK_HALFOP - 1] = (cfg_col_nick_halfop >= 0) ? cfg_col_nick_halfop & A_BOLD : 0;
        color_attr[COLOR_WIN_NICK_VOICE - 1] = (cfg_col_nick_voice >= 0) ? cfg_col_nick_voice & A_BOLD : 0;
        color_attr[COLOR_WIN_NICK_MORE - 1] = (cfg_col_nick_more >= 0) ? cfg_col_nick_more & A_BOLD : 0;
        color_attr[COLOR_WIN_NICK_SEP - 1] = 0;
        color_attr[COLOR_WIN_NICK_SELF - 1] = (cfg_col_nick_self >= 0) ? cfg_col_nick_self & A_BOLD : 0;
        color_attr[COLOR_WIN_NICK_PRIVATE - 1] = (cfg_col_nick_private >= 0) ? cfg_col_nick_private & A_BOLD : 0;
        color_attr[COLOR_DCC_SELECTED - 1] = (cfg_col_dcc_selected >= 0) ? cfg_col_dcc_selected & A_BOLD : 0;
        color_attr[COLOR_DCC_WAITING - 1] = (cfg_col_dcc_waiting >= 0) ? cfg_col_dcc_waiting & A_BOLD : 0;
        color_attr[COLOR_DCC_CONNECTING - 1] = (cfg_col_dcc_connecting >= 0) ? cfg_col_dcc_connecting & A_BOLD : 0;
        color_attr[COLOR_DCC_ACTIVE - 1] = (cfg_col_dcc_active >= 0) ? cfg_col_dcc_active & A_BOLD : 0;
        color_attr[COLOR_DCC_DONE - 1] = (cfg_col_dcc_done >= 0) ? cfg_col_dcc_done & A_BOLD : 0;
        color_attr[COLOR_DCC_FAILED - 1] = (cfg_col_dcc_failed >= 0) ? cfg_col_dcc_failed & A_BOLD : 0;
        color_attr[COLOR_DCC_ABORTED - 1] = (cfg_col_dcc_aborted >= 0) ? cfg_col_dcc_aborted & A_BOLD : 0;
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
    noecho ();
    nodelay (stdscr, TRUE);

    gui_init_colors ();
    
    gui_infobar = NULL;
    
    gui_ok = ((COLS > 5) && (LINES > 5));

    refresh ();
    
    /* init clipboard buffer */
    gui_input_clipboard = NULL;

    /* create new window/buffer */
    if (gui_window_new (0, 0, COLS, LINES))
    {
        gui_current_window = gui_windows;
        gui_buffer_new (gui_windows, NULL, NULL, 0, 1);
    
        signal (SIGWINCH, gui_refresh_screen);
    
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
    
    /* free clipboard buffer */
    if (gui_input_clipboard)
      free(gui_input_clipboard);

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
        gui_buffer_free (gui_buffers, 0);
    
    /* delete all windows */
    while (gui_windows)
        gui_window_free (gui_windows);
    
    /* delete global history */
    history_global_free ();
    
    /* delete infobar messages */
    while (gui_infobar)
        gui_infobar_remove ();
    
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
    char *ptr_string;
    
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
    ptr_string = buffer->last_line->last_message->message;
    while (ptr_string[0])
    {
        if ((ptr_string[0] > 0) && (ptr_string[0] < 32))
            ptr_string[0] = 32;
        ptr_string++;
    }
    length = utf8_strlen (message);
    buffer->last_line->length += length;
    if (type & MSG_TYPE_MSG)
        buffer->last_line->line_with_message = 1;
    if (type & MSG_TYPE_HIGHLIGHT)
        buffer->last_line->line_with_highlight = 1;
    if ((type & MSG_TYPE_TIME) || (type & MSG_TYPE_NICK) || (type & MSG_TYPE_PREFIX))
        buffer->last_line->length_align += length;
    if (type & MSG_TYPE_NOLOG)
        buffer->last_line->log_write = 0;
    if (pos)
    {
        pos[0] = '\n';
        if (buffer->num_displayed > 0)
            gui_draw_buffer_chat (buffer, 0);
        if (gui_add_hotlist && (buffer->num_displayed == 0))
        {
            if (3 - buffer->last_line->line_with_message -
                buffer->last_line->line_with_highlight <=
                buffer->notify_level)
            {
                if (buffer->last_line->line_with_highlight)
                    hotlist_add (HOTLIST_HIGHLIGHT, buffer);
                else if (BUFFER_IS_PRIVATE(buffer) && (buffer->last_line->line_with_message))
                    hotlist_add (HOTLIST_PRIVATE, buffer);
                else if (buffer->last_line->line_with_message)
                    hotlist_add (HOTLIST_MSG, buffer);
                else
                    hotlist_add (HOTLIST_LOW, buffer);
                gui_draw_buffer_status (gui_current_window->buffer, 1);
            }
        }
    }
    if (buffer->line_complete && buffer->log_file && buffer->last_line->log_write)
        log_write_line (buffer, buffer->last_line);
}

/*
 * gui_printf_internal: display a message in a buffer
 *                      This function should NEVER be called directly.
 *                      You should use macros defined in gui.h
 */

void
gui_printf_internal (t_gui_buffer *buffer, int display_time, int type, int color, char *message, ...)
{
    static char buf[8192];
    char text_time[1024];
    char text_time_char[2];
    time_t time_seconds;
    struct tm *local_time;
    int time_first_digit, time_last_digit;
    char *pos, *buf2, *buf3;
    int i, j;
    va_list argptr;
    static time_t seconds;
    struct tm *date_tmp;
    
    if (gui_init_ok)
    {
        if (color == -1)
            color = COLOR_WIN_CHAT;
    
        if (buffer == NULL)
        {
            type |= MSG_TYPE_NOLOG;
            if (SERVER(gui_current_window->buffer))
                buffer = SERVER(gui_current_window->buffer)->buffer;
            else
                buffer = gui_current_window->buffer;
            
            if (!buffer || buffer->dcc)
                buffer = gui_buffers;
        }
    
        if (buffer == NULL)
        {
            wee_log_printf ("WARNING: gui_printf_internal without buffer! This is a bug, "
                            "please send to developers - thanks\n");
            return;
        }
        
        if (buffer->dcc)
            buffer = gui_buffers;
        
        if (buffer->dcc)
            return;
    }
    
    va_start (argptr, message);
    vsnprintf (buf, sizeof (buf) - 1, message, argptr);
    va_end (argptr);
    
    if (!buf[0])
        return;
    
    if (cfg_look_remove_colors_from_msgs)
    {
        buf2 = (char *) malloc (strlen (buf) + 2);
        i = 0;
        j = 0;
        while (buf[i])
        {
            if (buf[i] == 0x02)
                i++;
            else
            {
                if (buf[i] == 0x03)
                {
                    if ((buf[i+1] >= '0') && (buf[i+1] <= '9')
                    && (buf[i+2] >= '0') && (buf[i+2] <= '9'))
                        i += 3;
                    else
                        i++;
                }
                else
                    buf2[j++] = buf[i++];
            }
        }
        buf2[j] = '\0';
    }
    else
        buf2 = strdup (buf);
    
    if (!local_utf8 || !utf8_is_valid (buf2))
        buf3 = weechat_convert_encoding ((local_utf8) ?
                                         cfg_look_charset_decode_iso : cfg_look_charset_decode_utf,
                                         (cfg_look_charset_internal && cfg_look_charset_internal[0]) ?
                                         cfg_look_charset_internal : local_charset,
                                         buf2);
    else
        buf3 = strdup (buf2);
    
    if (gui_init_ok)
    {
        seconds = time (NULL);
        date_tmp = localtime (&seconds);
        
        pos = buf3 - 1;
        while (pos)
        {
            if (display_time
                && cfg_look_buffer_timestamp && cfg_look_buffer_timestamp[0]
                && ((!buffer->last_line) || (buffer->line_complete)))
            {
                time_seconds = time (NULL);
                local_time = localtime (&time_seconds);
                strftime (text_time, sizeof (text_time), cfg_look_buffer_timestamp, local_time);
                
                time_first_digit = -1;
                time_last_digit = -1;
                i = 0;
                while (text_time[i])
                {
                    if (isdigit (text_time[i]))
                    {
                        if (time_first_digit == -1)
                            time_first_digit = i;
                        time_last_digit = i;
                    }
                    i++;
                }
                
                text_time_char[1] = '\0';
                i = 0;
                while (text_time[i])
                {
                    text_time_char[0] = text_time[i];
                    if (time_first_digit < 0)
                        gui_add_message (buffer, MSG_TYPE_TIME, COLOR_WIN_CHAT_TIME,
                                         text_time_char);
                    else
                    {
                        if ((i < time_first_digit) || (i > time_last_digit))
                            gui_add_message (buffer, MSG_TYPE_TIME, COLOR_WIN_CHAT_DARK,
                                             text_time_char);
                        else
                        {
                            if (isdigit (text_time[i]))
                                gui_add_message (buffer, MSG_TYPE_TIME, COLOR_WIN_CHAT_TIME,
                                                 text_time_char);
                            else
                                gui_add_message (buffer, MSG_TYPE_TIME, COLOR_WIN_CHAT_TIME_SEP,
                                                 text_time_char);
                        }
                    }
                    i++;
                }
                gui_add_message (buffer, MSG_TYPE_TIME, COLOR_WIN_CHAT_DARK, " ");
            }
            gui_add_message (buffer, type, color, pos + 1);
            pos = strchr (pos + 1, '\n');
            if (pos && !pos[1])
                pos = NULL;
        }
    }
    else
        printf ("%s", buf3);
    
    free (buf2);
    free (buf3);
}
