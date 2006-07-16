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

/* gui-curses-chat.c: chat display functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../common/utf8.h"
#include "../../common/weeconfig.h"
#include "../../irc/irc.h"
#include "gui-curses.h"


/*
 * gui_chat_set_style: set style (bold, underline, ..)
 *                     for a chat window
 */

void
gui_chat_set_style (t_gui_window *window, int style)
{
    wattron (GUI_CURSES(window)->win_chat, style);
}

/*
 * gui_chat_remove_style: remove style (bold, underline, ..)
 *                        for a chat window
 */

void
gui_chat_remove_style (t_gui_window *window, int style)
{
    wattroff (GUI_CURSES(window)->win_chat, style);
}

/*
 * gui_chat_toggle_style: toggle a style (bold, underline, ..)
 *                        for a chat window
 */

void
gui_chat_toggle_style (t_gui_window *window, int style)
{
    window->current_style_attr ^= style;
    if (window->current_style_attr & style)
        gui_chat_set_style (window, style);
    else
        gui_chat_remove_style (window, style);
}

/*
 * gui_chat_reset_style: reset style (color and attr)
 *                       for a chat window
 */

void
gui_chat_reset_style (t_gui_window *window)
{
    window->current_style_fg = -1;
    window->current_style_bg = -1;
    window->current_style_attr = 0;
    window->current_color_attr = 0;
    
    gui_window_set_weechat_color (GUI_CURSES(window)->win_chat, COLOR_WIN_CHAT);
    gui_chat_remove_style (window,
                           A_BOLD | A_UNDERLINE | A_REVERSE);
}

/*
 * gui_chat_set_color_style: set style for color
 */

void
gui_chat_set_color_style (t_gui_window *window, int style)
{
    window->current_color_attr |= style;
    wattron (GUI_CURSES(window)->win_chat, style);
}

/*
 * gui_chat_remove_color_style: remove style for color
 */

void
gui_chat_remove_color_style (t_gui_window *window, int style)
{
    window->current_color_attr &= !style;
    wattroff (GUI_CURSES(window)->win_chat, style);
}

/*
 * gui_chat_reset_color_style: reset style for color
 */

void
gui_chat_reset_color_style (t_gui_window *window)
{
    wattroff (GUI_CURSES(window)->win_chat, window->current_color_attr);
    window->current_color_attr = 0;
}

/*
 * gui_chat_set_color: set color for a chat window
 */

void
gui_chat_set_color (t_gui_window *window, int fg, int bg)
{
    if (((fg == -1) || (fg == 99))
        && ((bg == -1) || (bg == 99)))
        wattron (GUI_CURSES(window)->win_chat, COLOR_PAIR(63));
    else
    {
        if ((fg == -1) || (fg == 99))
            fg = WEECHAT_COLOR_WHITE;
        if ((bg == -1) || (bg == 99))
            bg = 0;
        wattron (GUI_CURSES(window)->win_chat, COLOR_PAIR((bg * 8) + fg));
    }
}

/*
 * gui_chat_set_weechat_color: set a WeeChat color for a chat window
 */

void
gui_chat_set_weechat_color (t_gui_window *window, int weechat_color)
{
    gui_chat_reset_style (window);
    gui_chat_set_style (window,
                        gui_color[weechat_color]->attributes);
    gui_chat_set_color (window,
                        gui_color[weechat_color]->foreground,
                        gui_color[weechat_color]->background);
}

/*
 * gui_chat_draw_title: draw title window for a buffer
 */

void
gui_chat_draw_title (t_gui_buffer *buffer, int erase)
{
    t_gui_window *ptr_win;
    char format[32], *buf, *buf2;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if ((ptr_win->buffer == buffer) && (buffer->num_displayed > 0))
        {
            if (erase)
                gui_window_curses_clear (GUI_CURSES(ptr_win)->win_title, COLOR_WIN_TITLE);
            
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_title, COLOR_WIN_TITLE);
            snprintf (format, 32, "%%-%ds", ptr_win->win_title_width);
            if (CHANNEL(buffer))
            {
                if (CHANNEL(buffer)->topic)
                {
                    buf = (char *)gui_color_decode ((unsigned char *)(CHANNEL(buffer)->topic), 0);
                    buf2 = channel_iconv_decode (SERVER(buffer),
                                                 CHANNEL(buffer),
                                                 (buf) ? buf : CHANNEL(buffer)->topic);
                    mvwprintw (GUI_CURSES(ptr_win)->win_title, 0, 0,
                               format, (buf2) ? buf2 : CHANNEL(buffer)->topic);
                    if (buf)
                        free (buf);
                    if (buf2)
                        free (buf2);
                }
                else
                    mvwprintw (GUI_CURSES(ptr_win)->win_title, 0, 0, format, " ");
            }
            else
            {
                if (buffer->type == BUFFER_TYPE_STANDARD)
                {
                    mvwprintw (GUI_CURSES(ptr_win)->win_title, 0, 0,
                               format,
                               PACKAGE_STRING " " WEECHAT_COPYRIGHT_DATE " - "
                               WEECHAT_WEBSITE);
                }
                else
                    mvwprintw (GUI_CURSES(ptr_win)->win_title, 0, 0, format, " ");
            }
            wnoutrefresh (GUI_CURSES(ptr_win)->win_title);
            refresh ();
        }
    }
}

/*
 * gui_chat_display_new_line: display a new line
 */

void
gui_chat_display_new_line (t_gui_window *window, int num_lines, int count,
                           int *lines_displayed, int simulate)
{
    if ((count == 0) || (*lines_displayed >= num_lines - count))
    {
        if ((!simulate) && (window->win_chat_cursor_x <= window->win_chat_width - 1))
        {
            wmove (GUI_CURSES(window)->win_chat,
                   window->win_chat_cursor_y,
                   window->win_chat_cursor_x);
            wclrtoeol (GUI_CURSES(window)->win_chat);
        }
        window->win_chat_cursor_y++;
    }
    window->win_chat_cursor_x = 0;
    (*lines_displayed)++;
}

/*
 * gui_chat_word_get_next_char: returns next char of a word
 *                              special chars like colors, bold, .. are skipped
 */

char *
gui_chat_word_get_next_char (t_gui_window *window, unsigned char *string,
                             int apply_style, int *width_screen)
{
    char str_fg[3], str_bg[3], utf_char[16];
    int fg, bg, weechat_color, char_size;

    if (width_screen)
        *width_screen = 0;
    
    while (string[0])
    {
        switch (string[0])
        {
            case GUI_ATTR_BOLD_CHAR:
                string++;
                if (apply_style)
                    gui_chat_toggle_style (window, A_BOLD);
                break;
            case GUI_ATTR_COLOR_CHAR:
                string++;
                str_fg[0] = '\0';
                str_bg[0] = '\0';
                fg = 99;
                bg = 99;
                if (isdigit (string[0]))
                {
                    str_fg[0] = string[0];
                    str_fg[1] = '\0';
                    string++;
                    if (isdigit (string[0]))
                    {
                        str_fg[1] = string[0];
                        str_fg[2] = '\0';
                        string++;
                    }
                }
                if (string[0] == ',')
                {
                    string++;
                    if (isdigit (string[0]))
                    {
                        str_bg[0] = string[0];
                        str_bg[1] = '\0';
                        string++;
                        if (isdigit (string[0]))
                        {
                            str_bg[1] = string[0];
                            str_bg[2] = '\0';
                            string++;
                        }
                    }
                }
                if (apply_style)
                {
                    if (str_fg[0] || str_bg[0])
                    {
                        if (str_fg[0])
                            sscanf (str_fg, "%d", &fg);
                        else
                            fg = window->current_style_fg;
                        if (str_bg[0])
                            sscanf (str_bg, "%d", &bg);
                        else
                            bg = window->current_style_bg;
                    }
                    if (!str_fg[0] && !str_bg[0])
                        gui_chat_reset_color_style (window);
                    window->current_style_fg = fg;
                    window->current_style_bg = bg;
                    gui_chat_set_color (window, fg, bg);
                }
                break;
            case GUI_ATTR_RESET_CHAR:
                string++;
                if (apply_style)
                    gui_chat_reset_style (window);
                break;
            case GUI_ATTR_FIXED_CHAR:
                string++;
                break;
            case GUI_ATTR_REVERSE_CHAR:
            case GUI_ATTR_REVERSE2_CHAR:
                string++;
                if (apply_style)
                    gui_chat_toggle_style (window, A_REVERSE);
                break;
            case GUI_ATTR_WEECHAT_COLOR_CHAR:
                string++;
                if (isdigit (string[0]) && isdigit (string[1]))
                {
                    str_fg[0] = string[0];
                    str_fg[1] = string[1];
                    str_fg[2] = '\0';
                    string += 2;
                    if (apply_style)
                    {
                        sscanf (str_fg, "%d", &weechat_color);
                        gui_chat_set_weechat_color (window, weechat_color);
                    }
                }
                break;
            case GUI_ATTR_WEECHAT_SET_CHAR:
                string++;
                switch (string[0])
                {
                    case GUI_ATTR_BOLD_CHAR:
                        string++;
                        if (apply_style)
                            gui_chat_set_color_style (window, A_BOLD);
                        break;
                    case GUI_ATTR_REVERSE_CHAR:
                    case GUI_ATTR_REVERSE2_CHAR:
                        string++;
                        if (apply_style)
                            gui_chat_set_color_style (window, A_REVERSE);
                        break;
                    case GUI_ATTR_UNDERLINE_CHAR:
                        string++;
                        if (apply_style)
                            gui_chat_set_color_style (window, A_UNDERLINE);
                        break;
                }
                break;
            case GUI_ATTR_WEECHAT_REMOVE_CHAR:
                string++;
                switch (string[0])
                {
                    case GUI_ATTR_BOLD_CHAR:
                        string++;
                        if (apply_style)
                            gui_chat_remove_color_style (window, A_BOLD);
                        break;
                    case GUI_ATTR_REVERSE_CHAR:
                    case GUI_ATTR_REVERSE2_CHAR:
                        string++;
                        if (apply_style)
                            gui_chat_remove_color_style (window, A_REVERSE);
                        break;
                    case GUI_ATTR_UNDERLINE_CHAR:
                        string++;
                        if (apply_style)
                            gui_chat_remove_color_style (window, A_UNDERLINE);
                        break;
                }
                break;
            case GUI_ATTR_ITALIC_CHAR:
                string++;
                break;
            case GUI_ATTR_UNDERLINE_CHAR:
                string++;
                if (apply_style)
                    gui_chat_toggle_style (window, A_UNDERLINE);
                break;
            default:
                if (string[0] < 32)
                    string++;
                else
                {
                    char_size = utf8_char_size ((char *) string);
                    if (width_screen)
                    {
                        memcpy (utf_char, string, char_size);
                        utf_char[char_size] = '\0';
                        *width_screen = utf8_width_screen (utf_char);
                    }
                    return (char *)string + char_size;
                }
        }
            
    }
    
    /* nothing found except color/attrib codes, so return NULL */
    return NULL;
}

/*
 * gui_chat_display_word_raw: display word on chat buffer, letter by letter
 *                            special chars like color, bold, .. are interpreted
 */

void
gui_chat_display_word_raw (t_gui_window *window, char *string)
{
    char *prev_char, *next_char, saved_char;
    
    wmove (GUI_CURSES(window)->win_chat,
           window->win_chat_cursor_y,
           window->win_chat_cursor_x);
    
    while (string && string[0])
    {
        next_char = gui_chat_word_get_next_char (window, (unsigned char *)string, 1, NULL);
        if (!next_char)
            return;
        
        prev_char = utf8_prev_char (string, next_char);
        if (prev_char)
        {
            saved_char = next_char[0];
            next_char[0] = '\0';
            if ((prev_char[0] == -110) && (!prev_char[1]))
                wprintw (GUI_CURSES(window)->win_chat, ".");
            else
                wprintw (GUI_CURSES(window)->win_chat, "%s", prev_char);
            next_char[0] = saved_char;
        }
        
        string = next_char;
    }
}

/*
 * gui_chat_display_word: display a word on chat buffer
 */

void
gui_chat_display_word (t_gui_window *window,
                       t_gui_line *line,
                       char *data,
                       char *end_offset,
                       int num_lines, int count, int *lines_displayed, int simulate)
{
    char *end_line, saved_char_end, saved_char;
    int pos_saved_char, chars_to_display, num_displayed;
    
    if (!data ||
        ((!simulate) && (window->win_chat_cursor_y >= window->win_chat_height)))
        return;
    
    end_line = data + strlen (data);
        
    if (end_offset[0])
    {
        saved_char_end = end_offset[1];
        end_offset[1] = '\0';
    }
    else
    {
        end_offset = NULL;
        saved_char_end = '\0';
    }
    
    while (data && data[0])
    {
        /* insert spaces for align text under time/nick */
        if ((line->length_align > 0) &&
            (window->win_chat_cursor_x == 0) &&
            (*lines_displayed > 0) &&
            /* TODO: modify arbitraty value for non aligning messages on time/nick? */
            (line->length_align < (window->win_chat_width - 5)))
        {
            if (!simulate)
            {
                wmove (GUI_CURSES(window)->win_chat,
                       window->win_chat_cursor_y,
                       window->win_chat_cursor_x);
                wclrtoeol (GUI_CURSES(window)->win_chat);
            }
            window->win_chat_cursor_x += line->length_align;
        }
        
        chars_to_display = gui_word_strlen (window, data);

        /* too long for current line */
        if (window->win_chat_cursor_x + chars_to_display > window->win_chat_width)
        {
            num_displayed = window->win_chat_width - window->win_chat_cursor_x;
            pos_saved_char = gui_word_real_pos (window, data, num_displayed);
            saved_char = data[pos_saved_char];
            data[pos_saved_char] = '\0';
            if ((!simulate) &&
                ((count == 0) || (*lines_displayed >= num_lines - count)))
                gui_chat_display_word_raw (window, data);
            data[pos_saved_char] = saved_char;
            data += pos_saved_char;
        }
        else
        {
            num_displayed = chars_to_display;
            if ((!simulate) &&
                ((count == 0) || (*lines_displayed >= num_lines - count)))
                gui_chat_display_word_raw (window, data);
            data += strlen (data);
        }
        
        window->win_chat_cursor_x += num_displayed;
        
        /* display new line? */
        if ((data >= end_line) ||
            (((simulate) ||
             (window->win_chat_cursor_y <= window->win_chat_height - 1)) &&
            (window->win_chat_cursor_x > (window->win_chat_width - 1))))
            gui_chat_display_new_line (window, num_lines, count,
                                       lines_displayed, simulate);
        
        if ((data >= end_line) ||
            ((!simulate) && (window->win_chat_cursor_y >= window->win_chat_height)))
            data = NULL;
    }
    
    if (end_offset)
        end_offset[1] = saved_char_end;
}

/*
 * gui_chat_get_word_info: returns info about next word: beginning, end, length
 */

void
gui_chat_get_word_info (t_gui_window *window,
                        char *data,
                        int *word_start_offset, int *word_end_offset,
                        int *word_length_with_spaces, int *word_length)
{
    char *start_data, *prev_char, *next_char;
    int leading_spaces, char_size;
    
    *word_start_offset = 0;
    *word_end_offset = 0;
    *word_length_with_spaces = 0;
    *word_length = 0;
    
    start_data = data;
    
    leading_spaces = 1;
    while (data && data[0])
    {
        next_char = gui_chat_word_get_next_char (window, (unsigned char *)data, 0, NULL);
        if (next_char)
        {
            prev_char = utf8_prev_char (data, next_char);
            if (prev_char)
            {
                if (prev_char[0] != ' ')
                {
                    if (leading_spaces)
                        *word_start_offset = prev_char - start_data;
                    leading_spaces = 0;
                    char_size = next_char - prev_char;
                    *word_end_offset = next_char - start_data - 1;
                    (*word_length_with_spaces) += char_size;
                    (*word_length) += char_size;
                }
                else
                {
                    if (leading_spaces)
                        (*word_length_with_spaces)++;
                    else
                    {
                        *word_end_offset = prev_char - start_data - 1;
                        return;
                    }
                }
                data = next_char;
            }
        }
        else
        {
            *word_end_offset = data + strlen (data) - start_data - 1;
            return;
        }
    }
}

/*
 * gui_chat_display_line: display a line in the chat window
 *                        if count == 0, display whole line
 *                        if count > 0, display 'count' lines
 *                          (beginning from the end)
 *                        if simulate == 1, nothing is displayed
 *                          (for counting how many lines would have been
 *                          lines displayed)
 *                        returns: number of lines displayed (or simulated)
 */

int
gui_chat_display_line (t_gui_window *window, t_gui_line *line, int count,
                       int simulate)
{
    int num_lines, x, y, lines_displayed;
    int read_marker_x, read_marker_y;
    int word_start_offset, word_end_offset;
    int word_length_with_spaces, word_length;
    char *ptr_data, *ptr_end_offset, *next_char, *prev_char;
    char *ptr_style, saved_char;
    
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
        num_lines = gui_chat_display_line (window, line, 0, 1);
        window->win_chat_cursor_x = x;
        window->win_chat_cursor_y = y;
    }
    
    /* calculate marker position (maybe not used for this line!) */
    if (line->ofs_after_date > 0)
    {
        saved_char = line->data[line->ofs_after_date - 1];
        line->data[line->ofs_after_date - 1] = '\0';
        read_marker_x = x + gui_word_strlen (NULL, line->data);
        line->data[line->ofs_after_date - 1] = saved_char;
    }
    else
        read_marker_x = x;
    read_marker_y = y;
    
    /* reset color & style for a new line */
    gui_chat_reset_style (window);
    
    lines_displayed = 0;
    ptr_data = line->data;
    while (ptr_data && ptr_data[0])
    {
        gui_chat_get_word_info (window,
                                ptr_data,
                                &word_start_offset,
                                &word_end_offset,
                                &word_length_with_spaces, &word_length);
        
        ptr_end_offset = ptr_data + word_end_offset;
        
        if (word_length > 0)
        {
            /* spaces + word too long for current line but ok for next line */
            if ((window->win_chat_cursor_x + word_length_with_spaces > window->win_chat_width)
                && (word_length <= window->win_chat_width - line->length_align))
            {
                gui_chat_display_new_line (window, num_lines, count,
                                           &lines_displayed, simulate);
                /* apply styles before jumping to start of word */
                if (!simulate && (word_start_offset > 0))
                {
                    saved_char = ptr_data[word_start_offset];
                    ptr_data[word_start_offset] = '\0';
                    ptr_style = ptr_data;
                    while ((ptr_style = gui_chat_word_get_next_char (window,
                                                                     (unsigned char *)ptr_style,
                                                                     1, NULL)) != NULL)
                    {
                        /* loop until no style/char available */
                    }
                    ptr_data[word_start_offset] = saved_char;
                }
                /* jump to start of word */
                ptr_data += word_start_offset;
            }
            
            /* display word */
            gui_chat_display_word (window, line, ptr_data,
                                   ptr_end_offset,
                                   num_lines, count, &lines_displayed, simulate);
            
            if ((!simulate) && (window->win_chat_cursor_y >= window->win_chat_height))
                ptr_data = NULL;
            else
            {
                /* move pointer after end of word */
                ptr_data = ptr_end_offset + 1;
                if (*(ptr_data - 1) == '\0')
                    ptr_data = NULL;
                
                if (window->win_chat_cursor_x == 0)
                {
                    while (ptr_data && (ptr_data[0] == ' '))
                    {
                        next_char = gui_chat_word_get_next_char (window,
                                                                 (unsigned char *)ptr_data,
                                                                 0, NULL);
                        if (!next_char)
                            break;
                        prev_char = utf8_prev_char (ptr_data, next_char);
                        if (prev_char && (prev_char[0] == ' '))
                            ptr_data = next_char;
                        else
                            break;
                    }
                }
            }
        }
        else
        {
            gui_chat_display_new_line (window, num_lines, count,
                                       &lines_displayed, simulate);
            ptr_data = NULL;
        }
    }
    
    if (simulate)
    {
        window->win_chat_cursor_x = x;
        window->win_chat_cursor_y = y;
    }
    else
    {
        /* display read marker if needed */
        if (cfg_look_read_marker && cfg_look_read_marker[0] &&
            window->buffer->last_read_line &&
            (window->buffer->last_read_line == line->prev_line))
        {
            gui_chat_set_weechat_color (window, COLOR_WIN_CHAT_READ_MARKER);
            mvwprintw (GUI_CURSES(window)->win_chat, read_marker_y, read_marker_x,
                       "%c", cfg_look_read_marker[0]);
        }
    }
    
    return lines_displayed;
}

/*
 * gui_chat_calculate_line_diff: returns pointer to line & offset for a difference
 *                               with given line
 */

void
gui_chat_calculate_line_diff (t_gui_window *window, t_gui_line **line,
                              int *line_pos, int difference)
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
            current_size = gui_chat_display_line (window, *line, 0, 1);
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
            current_size = gui_chat_display_line (window, *line, 0, 1);
        }
    }
    else
        current_size = gui_chat_display_line (window, *line, 0, 1);
    
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
                    current_size = gui_chat_display_line (window, *line, 0, 1);
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
                    current_size = gui_chat_display_line (window, *line, 0, 1);
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
 * gui_chat_draw: draw chat window for a buffer
 */

void
gui_chat_draw (t_gui_buffer *buffer, int erase)
{
    t_gui_window *ptr_win;
    t_gui_line *ptr_line;
    t_irc_dcc *dcc_first, *dcc_selected, *ptr_dcc;
    char format_empty[32];
    int i, j, line_pos, count, num_bars;
    unsigned long pct_complete;
    char *unit_name[] = { N_("bytes"), N_("Kb"), N_("Mb"), N_("Gb") };
    char *unit_format[] = { "%.0f", "%.1f", "%.02f", "%.02f" };
    float unit_divide[] = { 1, 1024, 1024*1024, 1024*1024*1024 };
    int num_unit;
    char format[32], date[128], *buf;
    struct tm *date_tmp;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if ((ptr_win->buffer == buffer) && (buffer->num_displayed > 0))
        {
            if (erase)
            {
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_chat, COLOR_WIN_CHAT);
                
                snprintf (format_empty, 32, "%%-%ds", ptr_win->win_chat_width);
                for (i = 0; i < ptr_win->win_chat_height; i++)
                {
                    mvwprintw (GUI_CURSES(ptr_win)->win_chat, i, 0, format_empty, " ");
                }
            }
            
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_chat, COLOR_WIN_CHAT);
            
            if (buffer->type == BUFFER_TYPE_DCC)
            {
                i = 0;
                dcc_first = (ptr_win->dcc_first) ? (t_irc_dcc *) ptr_win->dcc_first : dcc_list;
                dcc_selected = (ptr_win->dcc_selected) ? (t_irc_dcc *) ptr_win->dcc_selected : dcc_list;
                for (ptr_dcc = dcc_first; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
                {
                    if (i >= ptr_win->win_chat_height - 1)
                        break;
                    
                    /* nickname and filename */
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_chat,
                                                  (ptr_dcc == dcc_selected) ?
                                                  COLOR_DCC_SELECTED : COLOR_WIN_CHAT);
                    mvwprintw (GUI_CURSES(ptr_win)->win_chat, i, 0, "%s %-16s ",
                               (ptr_dcc == dcc_selected) ? "***" : "   ",
                               ptr_dcc->nick);
                    buf = channel_iconv_decode (SERVER(buffer),
                                                CHANNEL(buffer),
                                                (DCC_IS_CHAT(ptr_dcc->type)) ?
                                                _(ptr_dcc->filename) : ptr_dcc->filename);
                    wprintw (GUI_CURSES(ptr_win)->win_chat, "%s", buf);
                    free (buf);
                    if (DCC_IS_FILE(ptr_dcc->type))
                    {
                        if (ptr_dcc->filename_suffix > 0)
                            wprintw (GUI_CURSES(ptr_win)->win_chat, " (.%d)",
                                     ptr_dcc->filename_suffix);
                    }
                    
                    /* status */
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_chat,
                                                  (ptr_dcc == dcc_selected) ?
                                                  COLOR_DCC_SELECTED : COLOR_WIN_CHAT);
                    mvwprintw (GUI_CURSES(ptr_win)->win_chat, i + 1, 0, "%s %s ",
                               (ptr_dcc == dcc_selected) ? "***" : "   ",
                               (DCC_IS_RECV(ptr_dcc->type)) ? "-->>" : "<<--");
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_chat,
                                                  COLOR_DCC_WAITING + ptr_dcc->status);
                    buf = channel_iconv_decode (SERVER(buffer),
                                                CHANNEL(buffer),
                                                _(dcc_status_string[ptr_dcc->status]));
                    wprintw (GUI_CURSES(ptr_win)->win_chat, "%-10s", buf);
                    free (buf);
                    
                    /* other infos */
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_chat,
                                                  (ptr_dcc == dcc_selected) ?
                                                  COLOR_DCC_SELECTED : COLOR_WIN_CHAT);
                    if (DCC_IS_FILE(ptr_dcc->type))
                    {
                        wprintw (GUI_CURSES(ptr_win)->win_chat, "  [");
                        if (ptr_dcc->size == 0)
                        {
                            if (ptr_dcc->status == DCC_DONE)
                                num_bars = 10;
                            else
                                num_bars = 0;
                        }
                        else
                            num_bars = (int)((((float)(ptr_dcc->pos)/(float)(ptr_dcc->size))*100) / 10);
                        for (j = 0; j < num_bars - 1; j++)
                            wprintw (GUI_CURSES(ptr_win)->win_chat, "=");
                        if (num_bars > 0)
                            wprintw (GUI_CURSES(ptr_win)->win_chat, ">");
                        for (j = 0; j < 10 - num_bars; j++)
                            wprintw (GUI_CURSES(ptr_win)->win_chat, " ");
                        
                        if (ptr_dcc->size < 1024*10)
                            num_unit = 0;
                        else if (ptr_dcc->size < 1024*1024)
                            num_unit = 1;
                        else if (ptr_dcc->size < 1024*1024*1024)
                            num_unit = 2;
                        else
                            num_unit = 3;
                        if (ptr_dcc->size == 0)
                        {
                            if (ptr_dcc->status == DCC_DONE)
                                pct_complete = 100;
                            else
                                pct_complete = 0;
                        }
                        else
                            pct_complete = (unsigned long)(((float)(ptr_dcc->pos)/(float)(ptr_dcc->size))*100);
                        wprintw (GUI_CURSES(ptr_win)->win_chat, "] %3lu%%   ",
                                 pct_complete);
                        sprintf (format, "%s %%s / %s %%s",
                                 unit_format[num_unit],
                                 unit_format[num_unit]);
                        wprintw (GUI_CURSES(ptr_win)->win_chat, format,
                                 ((float)(ptr_dcc->pos)) / ((float)(unit_divide[num_unit])),
                                 unit_name[num_unit],
                                 ((float)(ptr_dcc->size)) / ((float)(unit_divide[num_unit])),
                                 unit_name[num_unit]);
                        
                        if (ptr_dcc->bytes_per_sec < 1024*1024)
                            num_unit = 1;
                        else if (ptr_dcc->bytes_per_sec < 1024*1024*1024)
                            num_unit = 2;
                        else
                            num_unit = 3;
                        wprintw (GUI_CURSES(ptr_win)->win_chat, "  (");
                        if (ptr_dcc->status == DCC_ACTIVE)
                        {
                            wprintw (GUI_CURSES(ptr_win)->win_chat, _("ETA"));
                            wprintw (GUI_CURSES(ptr_win)->win_chat, ": %.2lu:%.2lu:%.2lu - ",
                                     ptr_dcc->eta / 3600,
                                     (ptr_dcc->eta / 60) % 60,
                                     ptr_dcc->eta % 60);
                        }
                        sprintf (format, "%s %%s/s)", unit_format[num_unit]);
                        buf = channel_iconv_decode (SERVER(buffer),
                                                    CHANNEL(buffer),
                                                    unit_name[num_unit]);
                        wprintw (GUI_CURSES(ptr_win)->win_chat, format,
                                 ((float)ptr_dcc->bytes_per_sec) / ((float)(unit_divide[num_unit])),
                                 buf);
                        free (buf);
                    }
                    else
                    {
                        date_tmp = localtime (&(ptr_dcc->start_time));
                        strftime (date, sizeof (date) - 1, "%a, %d %b %Y %H:%M:%S", date_tmp);
                        wprintw (GUI_CURSES(ptr_win)->win_chat, "  %s", date);
                    }
                    
                    wclrtoeol (GUI_CURSES(ptr_win)->win_chat);
                    
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
                    /* look for first line to display, starting from last line */
                    ptr_line = NULL;
                    line_pos = 0;
                    gui_chat_calculate_line_diff (ptr_win, &ptr_line, &line_pos,
                                                  (-1) * (ptr_win->win_chat_height - 1));
                }

                if (line_pos > 0)
                {
                    /* display end of first line at top of screen */
                    gui_chat_display_line (ptr_win, ptr_line,
                                           gui_chat_display_line (ptr_win,
                                                                  ptr_line,
                                                                  0, 1) -
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
                    count = gui_chat_display_line (ptr_win, ptr_line, 0, 0);
                    ptr_line = ptr_line->next_line;
                }
                
                ptr_win->scroll = (ptr_win->win_chat_cursor_y > ptr_win->win_chat_height - 1);
                
                /* check if last line of buffer is entirely displayed and scrolling */
                /* if so, disable scroll indicator */
                if (!ptr_line && ptr_win->scroll)
                {
                    if (count == gui_chat_display_line (ptr_win, ptr_win->buffer->last_line, 0, 1))
                    {
                        ptr_win->scroll = 0;
                        ptr_win->start_line = NULL;
                        ptr_win->start_line_pos = 0;
                    }
                }
                
                if (!ptr_win->scroll && (ptr_win->start_line == ptr_win->buffer->lines))
                {
                    ptr_win->start_line = NULL;
                    ptr_win->start_line_pos = 0;
                }
                
                /* cursor is below end line of chat window? */
                if (ptr_win->win_chat_cursor_y > ptr_win->win_chat_height - 1)
                {
                    ptr_win->win_chat_cursor_x = 0;
                    ptr_win->win_chat_cursor_y = ptr_win->win_chat_height - 1;
                }
            }
            wnoutrefresh (GUI_CURSES(ptr_win)->win_chat);
            refresh ();
        }
    }
}

/*
 * gui_chat_draw_line: add a line to chat window for a buffer
 */

void
gui_chat_draw_line (t_gui_buffer *buffer, t_gui_line *line)
{
    /* This function does nothing in Curses GUI,
       line will be displayed by gui_buffer_draw_chat()  */
    (void) buffer;
    (void) line;
}
