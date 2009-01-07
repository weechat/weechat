/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
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

/* gui-gtk-chat.c: chat display functions for Gtk GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-utf8.h"
#include "../gui-buffer.h"
#include "../gui-chat.h"
#include "../gui-color.h"
#include "../gui-main.h"
#include "../gui-window.h"
#include "gui-gtk.h"


/*
 * gui_chat_set_style: set style (bold, underline, ..)
 *                     for a chat window
 */

void
gui_chat_set_style (struct t_gui_window *window, int style)
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
gui_chat_remove_style (struct t_gui_window *window, int style)
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
gui_chat_toggle_style (struct t_gui_window *window, int style)
{
    GUI_WINDOW_OBJECTS(window)->current_style_attr ^= style;
    if (GUI_WINDOW_OBJECTS(window)->current_style_attr & style)
        gui_chat_set_style (window, style);
    else
        gui_chat_remove_style (window, style);
}

/*
 * gui_chat_reset_style: reset style (color and attr)
 *                       for a chat window
 */

void
gui_chat_reset_style (struct t_gui_window *window)
{
    GUI_WINDOW_OBJECTS(window)->current_style_fg = -1;
    GUI_WINDOW_OBJECTS(window)->current_style_bg = -1;
    GUI_WINDOW_OBJECTS(window)->current_style_attr = 0;
    GUI_WINDOW_OBJECTS(window)->current_color_attr = 0;
    
    /* TODO: change following function call */
    /*gui_window_set_weechat_color (window->win_chat, COLOR_WIN_CHAT);*/
    gui_chat_remove_style (window,
                           A_BOLD | A_UNDERLINE | A_REVERSE);
}

/*
 * gui_chat_set_color_style: set style for color
 */

void
gui_chat_set_color_style (struct t_gui_window *window, int style)
{
    GUI_WINDOW_OBJECTS(window)->current_color_attr |= style;
    /* TODO: change following function call */
    /*wattron (window->win_chat, style);*/
}

/*
 * gui_chat_remove_color_style: remove style for color
 */

void
gui_chat_remove_color_style (struct t_gui_window *window, int style)
{
    GUI_WINDOW_OBJECTS(window)->current_color_attr &= !style;
    /* TODO: change following function call */
    /*wattroff (window->win_chat, style);*/
}

/*
 * gui_chat_reset_color_style: reset style for color
 */

void
gui_chat_reset_color_style (struct t_gui_window *window)
{
    /* TODO: change following function call */
    /*wattroff (window->win_chat, window->current_color_attr);*/
    GUI_WINDOW_OBJECTS(window)->current_color_attr = 0;
}

/*
 * gui_chat_set_color: set color for a chat window
 */

void
gui_chat_set_color (struct t_gui_window *window, int fg, int bg)
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
gui_chat_set_weechat_color (struct t_gui_window *window, int weechat_color)
{
    gui_chat_reset_style (window);
    gui_chat_set_style (window,
                        gui_color[weechat_color]->attributes);
    gui_chat_set_color (window,
                        gui_color[weechat_color]->foreground,
                        gui_color[weechat_color]->background);
}

/*
 * gui_chat_string_next_char: returns next char of a word (for display)
 *                            special chars like colors, bold, .. are skipped
 *                            and optionaly applied
 */

char *
gui_chat_string_next_char (struct t_gui_window *window,
                           const unsigned char *string, int apply_style)
{
    char str_fg[3];
    int weechat_color;
    
    while (string[0])
    {
        switch (string[0])
        {
            case GUI_COLOR_RESET_CHAR:
                string++;
                if (apply_style)
                    gui_chat_reset_style (window);
                break;
            case GUI_COLOR_COLOR_CHAR:
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
            case GUI_COLOR_SET_WEECHAT_CHAR:
                string++;
                switch (string[0])
                {
                    case GUI_COLOR_ATTR_BOLD_CHAR:
                        string++;
                        if (apply_style)
                            gui_chat_set_color_style (window, A_BOLD);
                        break;
                    case GUI_COLOR_ATTR_REVERSE_CHAR:
                        string++;
                        if (apply_style)
                            gui_chat_set_color_style (window, A_REVERSE);
                        break;
                    case GUI_COLOR_ATTR_ITALIC_CHAR:
                        /* not available in Curses GUI */
                        string++;
                        break;
                    case GUI_COLOR_ATTR_UNDERLINE_CHAR:
                        string++;
                        if (apply_style)
                            gui_chat_set_color_style (window, A_UNDERLINE);
                        break;
                }
                break;
            case GUI_COLOR_REMOVE_WEECHAT_CHAR:
                string++;
                switch (string[0])
                {
                    case GUI_COLOR_ATTR_BOLD_CHAR:
                        string++;
                        if (apply_style)
                            gui_chat_remove_color_style (window, A_BOLD);
                        break;
                    case GUI_COLOR_ATTR_REVERSE_CHAR:
                        string++;
                        if (apply_style)
                            gui_chat_remove_color_style (window, A_REVERSE);
                        break;
                    case GUI_COLOR_ATTR_ITALIC_CHAR:
                        /* not available in Curses GUI */
                        string++;
                        break;
                    case GUI_COLOR_ATTR_UNDERLINE_CHAR:
                        string++;
                        if (apply_style)
                            gui_chat_remove_color_style (window, A_UNDERLINE);
                        break;
                }
                break;
            default:
                if (string[0] < 32)
                    string++;
                else
                    return (char *)string;
                break;
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
gui_chat_display_word_raw (struct t_gui_window *window, const char *string)
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
gui_chat_display_word (struct t_gui_window *window,
                       struct t_gui_line *line,
                       const char *data,
                       const char *end_offset,
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
 * gui_chat_display_line: display a line in the chat window
 *                        if count == 0, display whole line
 *                        if count > 0, display 'count' lines (beginning from the end)
 *                        if simulate == 1, nothing is displayed (for counting how
 *                                          many lines would have been lines displayed)
 *                        returns: number of lines displayed (or simulated)
 */

int
gui_chat_display_line (struct t_gui_window *window, struct t_gui_line *line, int count,
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
gui_chat_calculate_line_diff (struct t_gui_window *window, struct t_gui_line **line,
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
gui_chat_draw (struct t_gui_buffer *buffer, int erase)
{
    /*struct t_gui_window *ptr_win;
    struct t_gui_line *ptr_line;
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
gui_chat_draw_line (struct t_gui_buffer *buffer, struct t_gui_line *line)
{
    //struct t_gui_window *ptr_win;
    //unsigned char *message_without_color;
    //GtkTextIter start, end;

    (void) buffer;
    (void) line;

    /*
    ptr_win = gui_buffer_find_window (buffer);
    if (ptr_win)
    {
        message_without_color = gui_color_decode ((unsigned char *)(line->message));
        if (message_without_color)
        {
            gtk_text_buffer_insert_at_cursor (GUI_WINDOW_OBJECTS(ptr_win)->textbuffer_chat,
                                              (char *)message_without_color, -1);
            gtk_text_buffer_insert_at_cursor (GUI_WINDOW_OBJECTS(ptr_win)->textbuffer_chat,
                                              "\n", -1);
            gtk_text_buffer_get_bounds (GUI_WINDOW_OBJECTS(ptr_win)->textbuffer_chat,
                                        &start, &end);
            // TODO
            //gtk_text_buffer_apply_tag (ptr_win->textbuffer_chat, ptr_win->texttag_chat, &start, &end);
            free (message_without_color);
        }
    }
    */
}
