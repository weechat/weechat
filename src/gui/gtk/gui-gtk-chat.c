/*
 * gui-gtk-chat.c - chat display functions for Gtk GUI
 *
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

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
#include "../gui-line.h"
#include "../gui-window.h"
#include "gui-gtk.h"


/*
 * Sets style (bold, underline, ..) for a chat window.
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
 * Removes style (bold, underline, ..) for a chat window.
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
 * Toggles a style (bold, underline, ..) for a chat window.
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
 * Resets style (color and attr) for a chat window.
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
 * Sets style for color.
 */

void
gui_chat_set_color_style (struct t_gui_window *window, int style)
{
    GUI_WINDOW_OBJECTS(window)->current_color_attr |= style;
    /* TODO: change following function call */
    /*wattron (window->win_chat, style);*/
}

/*
 * Removes style for color.
 */

void
gui_chat_remove_color_style (struct t_gui_window *window, int style)
{
    GUI_WINDOW_OBJECTS(window)->current_color_attr &= !style;
    /* TODO: change following function call */
    /*wattroff (window->win_chat, style);*/
}

/*
 * Resets style for color.
 */

void
gui_chat_reset_color_style (struct t_gui_window *window)
{
    /* TODO: change following function call */
    /*wattroff (window->win_chat, window->current_color_attr);*/
    GUI_WINDOW_OBJECTS(window)->current_color_attr = 0;
}

/*
 * Sets color for a chat window.
 */

void
gui_chat_set_color (struct t_gui_window *window, int fg, int bg)
{
    /* TODO: write this function for Gtk */
    (void) window;
    (void) fg;
    (void) bg;
}

/*
 * Sets a WeeChat color for a chat window.
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
 * Returns next char of a word (for display), special chars like
 * colors/attributes are skipped and optionally applied.
 */

char *
gui_chat_string_next_char (struct t_gui_window *window, struct t_gui_line *line,
                           const unsigned char *string, int apply_style,
                           int apply_style_inactive,
                           int nick_offline)
{
    /* TODO: write this function for Gtk */
    (void) window;
    (void) line;
    (void) apply_style;
    (void) apply_style_inactive;
    (void) nick_offline;

    return (char *)string;
}

/*
 * Display word on chat buffer, letter by letter, special chars like
 * colors/attributes are interpreted.
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
 * Displays a word on chat buffer.
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
 * Displays a line in the chat window.
 *
 * If count == 0, display whole line.
 * If count > 0, display 'count' lines (beginning from the end).
 * If simulate == 1, nothing is displayed (for counting how many lines would
 * have been lines displayed).
 *
 * Returns number of lines displayed (or simulated).
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
 * Returns pointer to line & offset for a difference with given line.
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
            *line = window->buffer->lines->last_line;
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
            *line = window->buffer->lines->first_line;
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
            *line = window->buffer->lines->first_line;
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
 * Draws chat window for a buffer.
 */

void
gui_chat_draw (struct t_gui_buffer *buffer, int clear_chat)
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

    if (!gui_init_ok)
        return;

    /* TODO: write this function for Gtk */
    (void) buffer;
    (void) clear_chat;
}
