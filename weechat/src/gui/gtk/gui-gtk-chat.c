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

/* gui-gtk-chat.c: chat display functions for Gtk GUI */


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
#include "gui-gtk.h"


/*
 * gui_chat_set_style: set style (bold, underline, ..)
 *                     for a chat window
 */

void
gui_chat_set_style (t_gui_window *window, int style)
{
    /* TODO: write this function for Gtk */
    /*wattron (window->win_chat, style);*/
    (void) window;
    (void) style;
}

/*
 * gui_chat_remove_style: remove style (bold, underline, ..)
 *                        for a chat window
 */

void
gui_chat_remove_style (t_gui_window *window, int style)
{
    /* TODO: write this function for Gtk */
    /*wattroff (window->win_chat, style);*/
    (void) window;
    (void) style;
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
    
    /* TODO: change following function call */
    /*gui_window_set_weechat_color (window->win_chat, COLOR_WIN_CHAT);*/
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
    /* TODO: change following function call */
    /*wattron (window->win_chat, style);*/
}

/*
 * gui_chat_remove_color_style: remove style for color
 */

void
gui_chat_remove_color_style (t_gui_window *window, int style)
{
    window->current_color_attr &= !style;
    /* TODO: change following function call */
    /*wattroff (window->win_chat, style);*/
}

/*
 * gui_chat_reset_color_style: reset style for color
 */

void
gui_chat_reset_color_style (t_gui_window *window)
{
    /* TODO: change following function call */
    /*wattroff (window->win_chat, window->current_color_attr);*/
    window->current_color_attr = 0;
}

/*
 * gui_chat_set_color: set color for a chat window
 */

void
gui_chat_set_color (t_gui_window *window, int fg, int bg)
{
    /* TODO: write this function for Gtk */    
    /*if (((fg == -1) || (fg == 99))
        && ((bg == -1) || (bg == 99)))
        wattron (window->win_chat, COLOR_PAIR(63));
    else
    {
        if ((fg == -1) || (fg == 99))
            fg = WEECHAT_COLOR_WHITE;
        if ((bg == -1) || (bg == 99))
            bg = 0;
        wattron (window->win_chat, COLOR_PAIR((bg * 8) + fg));
    }*/
    (void) window;
    (void) fg;
    (void) bg;
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
    /* TODO: write this function for Gtk */
    (void) buffer;
    (void) erase;
}

/*
 * gui_chat_word_get_next_char: returns next char of a word
 *                              special chars like colors, bold, .. are skipped
 */

char *
gui_chat_word_get_next_char (t_gui_window *window, unsigned char *string, int apply_style)
{
    char str_fg[3], str_bg[3];
    int fg, bg, weechat_color;
    
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
                    return utf8_next_char ((char *)string);
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
    /*char *prev_char, *next_char, saved_char;*/
    
    /* TODO: write this function for Gtk */
    (void) window;
    (void) string;
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
    /*char *end_line, saved_char_end, saved_char;
      int pos_saved_char, chars_to_display, num_displayed;*/
    
    /* TODO: write this function for Gtk */
    (void) window;
    (void) line;
    (void) data;
    (void) end_offset;
    (void) num_lines;
    (void) count;
    (void) lines_displayed;
    (void) simulate;
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
        next_char = gui_chat_word_get_next_char (window, (unsigned char *)data, 0);
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
 *                        if count > 0, display 'count' lines (beginning from the end)
 *                        if simulate == 1, nothing is displayed (for counting how
 *                                          many lines would have been lines displayed)
 *                        returns: number of lines displayed (or simulated)
 */

int
gui_chat_display_line (t_gui_window *window, t_gui_line *line, int count,
                       int simulate)
{
    /* TODO: write this function for Gtk */
    (void) window;
    (void) line;
    (void) count;
    (void) simulate;
    return 1;
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
    /*t_gui_window *ptr_win;
    t_gui_line *ptr_line;
    t_irc_dcc *dcc_first, *dcc_selected, *ptr_dcc;
    char format_empty[32];
    int i, j, line_pos, count, num_bars;
    char *unit_name[] = { N_("bytes"), N_("Kb"), N_("Mb"), N_("Gb") };
    char *unit_format[] = { "%.0Lf", "%.1Lf", "%.02Lf", "%.02Lf" };
    long unit_divide[] = { 1, 1024, 1024*1024, 1024*1024,1024 };
    int num_unit;
    char format[32], date[128], *buf;
    struct tm *date_tmp;*/
    
    if (!gui_ok)
        return;
    
    /* TODO: write this function for Gtk */
    (void) buffer;
    (void) erase;
}

/*
 * gui_chat_draw_line: add a line to chat window for a buffer
 */

void
gui_chat_draw_line (t_gui_buffer *buffer, t_gui_line *line)
{
    t_gui_window *ptr_win;
    unsigned char *text_without_color;
    GtkTextIter start, end;
    
    ptr_win = gui_buffer_find_window (buffer);
    if (ptr_win)
    {
        text_without_color = gui_color_decode ((unsigned char *)(line->data), 0);
        if (text_without_color)
        {
            gtk_text_buffer_insert_at_cursor (GUI_GTK(ptr_win)->textbuffer_chat,
                                              (char *)text_without_color, -1);
            gtk_text_buffer_insert_at_cursor (GUI_GTK(ptr_win)->textbuffer_chat,
                                              "\n", -1);
            gtk_text_buffer_get_bounds (GUI_GTK(ptr_win)->textbuffer_chat,
                                        &start, &end);
            /* TODO */
            /*gtk_text_buffer_apply_tag (ptr_win->textbuffer_chat, ptr_win->texttag_chat, &start, &end);*/
            free (text_without_color);
        }
    }
}
