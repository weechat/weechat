/*
 * gui-curses-bar-window.c - bar window functions for Curses GUI
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "../../core/weechat.h"
#include "../../core/core-config.h"
#include "../../core/core-log.h"
#include "../../core/core-string.h"
#include "../../core/core-utf8.h"
#include "../../plugins/plugin.h"
#include "../gui-bar.h"
#include "../gui-bar-item.h"
#include "../gui-bar-window.h"
#include "../gui-chat.h"
#include "../gui-color.h"
#include "../gui-cursor.h"
#include "../gui-window.h"
#include "gui-curses.h"
#include "gui-curses-bar-window.h"
#include "gui-curses-color.h"
#include "gui-curses-window.h"


/*
 * Initializes Curses windows for bar window.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
gui_bar_window_objects_init (struct t_gui_bar_window *bar_window)
{
    struct t_gui_bar_window_curses_objects *new_objects;

    if (!bar_window)
        return 0;

    new_objects = malloc (sizeof (*new_objects));
    if (new_objects)
    {
        bar_window->gui_objects = new_objects;
        GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar = NULL;
        GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator = NULL;
        return 1;
    }
    return 0;
}

/*
 * Frees Curses windows for a bar window.
 */

void
gui_bar_window_objects_free (struct t_gui_bar_window *bar_window)
{
    if (!bar_window)
        return;

    if (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar)
    {
        delwin (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar);
        GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar = NULL;
    }
    if (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator)
    {
        delwin (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator);
        GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator = NULL;
    }
}

/*
 * Creates curses window for bar.
 */

void
gui_bar_window_create_win (struct t_gui_bar_window *bar_window)
{
    if (!bar_window
        || CONFIG_BOOLEAN(bar_window->bar->options[GUI_BAR_OPTION_HIDDEN]))
    {
        return;
    }

    if (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar)
    {
        delwin (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar);
        GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar = NULL;
    }
    if (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator)
    {
        delwin (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator);
        GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator = NULL;
    }

    if ((bar_window->x >= 0) && (bar_window->y >= 0))
    {
        GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar = newwin (bar_window->height,
                                                              bar_window->width,
                                                              bar_window->y,
                                                              bar_window->x);

        if (CONFIG_INTEGER(bar_window->bar->options[GUI_BAR_OPTION_SEPARATOR]))
        {
            switch (CONFIG_ENUM(bar_window->bar->options[GUI_BAR_OPTION_POSITION]))
            {
                case GUI_BAR_POSITION_BOTTOM:
                    GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator =
                        newwin (1, bar_window->width,
                                bar_window->y - 1, bar_window->x);
                    break;
                case GUI_BAR_POSITION_TOP:
                    GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator =
                        newwin (1, bar_window->width,
                                bar_window->y + bar_window->height, bar_window->x);
                    break;
                case GUI_BAR_POSITION_LEFT:
                    GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator =
                        newwin (bar_window->height, 1,
                                bar_window->y, bar_window->x + bar_window->width);
                    break;
                case GUI_BAR_POSITION_RIGHT:
                    GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator =
                        newwin (bar_window->height, 1,
                                bar_window->y, bar_window->x - 1);
                    break;
                case GUI_BAR_NUM_POSITIONS:
                    break;
            }
        }
    }
}

/*
 * Prints a string text on a bar window.
 *
 * Returns:
 *   1: everything was printed
 *   0: some text was not displayed (wrapped due to bar window width)
 */

int
gui_bar_window_print_string (struct t_gui_bar_window *bar_window,
                             struct t_gui_window *window,
                             enum t_gui_bar_filling filling,
                             int *x, int *y,
                             const char *string,
                             int reset_color_before_display,
                             int hide_chars_if_scrolling,
                             int *index_item, int *index_subitem, int *index_line)
{
    int x_with_hidden, size_on_screen, reverse_video, hidden, color_bg;
    char utf_char[16], utf_char2[16], *output;
    const char *ptr_char;

    if (!bar_window || !string || !string[0])
        return 1;

    wmove (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar, *y, *x);

    color_bg = ((CONFIG_ENUM(bar_window->bar->options[GUI_BAR_OPTION_TYPE]) != GUI_BAR_TYPE_ROOT)
                && (window != gui_current_window)) ?
        GUI_BAR_OPTION_COLOR_BG_INACTIVE : GUI_BAR_OPTION_COLOR_BG;

    if (reset_color_before_display)
    {
        gui_window_set_custom_color_fg_bg (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                           CONFIG_COLOR(bar_window->bar->options[GUI_BAR_OPTION_COLOR_FG]),
                                           CONFIG_COLOR(bar_window->bar->options[color_bg]),
                                           1);
    }

    x_with_hidden = *x;

    hidden = 0;

    while (string && string[0])
    {
        switch (string[0])
        {
            case GUI_COLOR_COLOR_CHAR:
                string++;
                switch (string[0])
                {
                    case GUI_COLOR_FG_CHAR: /* fg color */
                        string++;
                        gui_window_string_apply_color_fg ((unsigned char **)&string,
                                                          GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar);
                        break;
                    case GUI_COLOR_BG_CHAR: /* bg color */
                        string++;
                        gui_window_string_apply_color_bg ((unsigned char **)&string,
                                                          GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar);
                        break;
                    case GUI_COLOR_FG_BG_CHAR: /* fg + bg color */
                        string++;
                        gui_window_string_apply_color_fg_bg ((unsigned char **)&string,
                                                             GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar);
                        break;
                    case GUI_COLOR_EXTENDED_CHAR: /* pair number */
                        string++;
                        gui_window_string_apply_color_pair ((unsigned char **)&string,
                                                            GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar);
                        break;
                    case GUI_COLOR_EMPHASIS_CHAR: /* emphasis */
                        string++;
                        gui_window_toggle_emphasis ();
                        break;
                    case GUI_COLOR_BAR_CHAR: /* bar color */
                        switch (string[1])
                        {
                            case GUI_COLOR_BAR_FG_CHAR:
                                /* bar foreground */
                                string += 2;
                                gui_window_set_custom_color_fg (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                                CONFIG_COLOR(bar_window->bar->options[GUI_BAR_OPTION_COLOR_FG]));
                                break;
                            case GUI_COLOR_BAR_DELIM_CHAR:
                                /* bar delimiter */
                                string += 2;
                                gui_window_set_custom_color_fg (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                                CONFIG_COLOR(bar_window->bar->options[GUI_BAR_OPTION_COLOR_DELIM]));
                                break;
                            case GUI_COLOR_BAR_BG_CHAR:
                                /* bar background */
                                string += 2;
                                gui_window_set_custom_color_bg (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                                CONFIG_COLOR(bar_window->bar->options[color_bg]));
                                break;
                            case GUI_COLOR_BAR_START_INPUT_CHAR:
                                string += 2;
                                hidden = 0;
                                break;
                            case GUI_COLOR_BAR_START_INPUT_HIDDEN_CHAR:
                                string += 2;
                                hidden = 1;
                                break;
                            case GUI_COLOR_BAR_MOVE_CURSOR_CHAR:
                                /* move cursor to current position on screen */
                                string += 2;
                                getyx (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                       bar_window->cursor_y,
                                       bar_window->cursor_x);
                                bar_window->cursor_x += bar_window->x;
                                bar_window->cursor_y += bar_window->y;
                                break;
                            case GUI_COLOR_BAR_START_ITEM:
                                string += 2;
                                if (*index_item < 0)
                                {
                                    *index_item = 0;
                                    *index_subitem = 0;
                                }
                                else
                                {
                                    (*index_subitem)++;
                                    if (*index_subitem >= bar_window->items_subcount[*index_item])
                                    {
                                        (*index_item)++;
                                        *index_subitem = 0;
                                    }
                                }
                                *index_line = 0;
                                gui_bar_window_coords_add (bar_window,
                                                           (*index_item >= bar_window->items_count) ? -1 : *index_item,
                                                           (*index_item >= bar_window->items_count) ? -1 : *index_subitem,
                                                           *index_line,
                                                           *x + bar_window->x,
                                                           *y + bar_window->y);
                                break;
                            case GUI_COLOR_BAR_START_LINE_ITEM:
                                string += 2;
                                (*index_line)++;
                                gui_bar_window_coords_add (bar_window,
                                                           *index_item,
                                                           *index_subitem,
                                                           *index_line,
                                                           *x + bar_window->x,
                                                           *y + bar_window->y);
                                break;
                            case GUI_COLOR_BAR_SPACER:
                                string += 2;
                                break;
                            default:
                                string++;
                                break;
                        }
                        break;
                    case GUI_COLOR_RESET_CHAR: /* reset color (keep attributes) */
                        string++;
                        gui_window_set_custom_color_fg_bg (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                           CONFIG_COLOR(bar_window->bar->options[GUI_BAR_OPTION_COLOR_FG]),
                                                           CONFIG_COLOR(bar_window->bar->options[color_bg]),
                                                           0);
                        break;
                    default:
                        gui_window_string_apply_color_weechat ((unsigned char **)&string,
                                                               GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar);
                        break;
                }
                break;
            case GUI_COLOR_SET_ATTR_CHAR:
                string++;
                gui_window_string_apply_color_set_attr ((unsigned char **)&string,
                                                        GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar);
                break;
            case GUI_COLOR_REMOVE_ATTR_CHAR:
                string++;
                gui_window_string_apply_color_remove_attr ((unsigned char **)&string,
                                                           GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar);
                break;
            case GUI_COLOR_RESET_CHAR:
                string++;
                gui_window_remove_color_style (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                               A_ALL_ATTR);
                gui_window_set_custom_color_fg_bg (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                   CONFIG_COLOR(bar_window->bar->options[GUI_BAR_OPTION_COLOR_FG]),
                                                   CONFIG_COLOR(bar_window->bar->options[color_bg]),
                                                   1);
                break;
            default:
                utf8_strncpy (utf_char, string, 1);
                reverse_video = 0;
                ptr_char = utf_char;
                if (utf_char[0] == '\t')
                {
                    /* expand tabulation with spaces */
                    ptr_char = config_tab_spaces;
                }
                else if (((unsigned char)utf_char[0]) < 32)
                {
                    /*
                     * display chars < 32 with letter/symbol
                     * and set reverse video (if not already enabled)
                     */
                    snprintf (utf_char, sizeof (utf_char), "%c",
                              'A' + ((unsigned char)utf_char[0]) - 1);
                    reverse_video = (gui_window_current_color_attr & A_REVERSE) ?
                        0 : 1;
                }

                while (ptr_char && ptr_char[0])
                {
                    utf8_strncpy (utf_char2, ptr_char, 1);
                    size_on_screen = utf8_char_size_screen (utf_char2);
                    if (size_on_screen >= 0)
                    {
                        if (hide_chars_if_scrolling
                            && (x_with_hidden < bar_window->scroll_x))
                        {
                            /* hidden char (before scroll_x value) */
                            x_with_hidden++;
                        }
                        else if (!hidden)
                        {
                            if (*x + size_on_screen > bar_window->width)
                            {
                                if (filling == GUI_BAR_FILLING_VERTICAL)
                                    return 1;
                                if (*y >= bar_window->height - 1)
                                    return 0;
                                *x = 0;
                                (*y)++;
                                wmove (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar, *y, *x);
                            }

                            output = string_iconv_from_internal (NULL, utf_char2);
                            if (reverse_video)
                                wattron (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar, A_REVERSE);
                            waddstr (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                     (output) ? output : utf_char2);
                            if (reverse_video)
                                wattroff (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar, A_REVERSE);
                            free (output);

                            if (gui_window_current_emphasis)
                            {
                                gui_window_emphasize (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                      *x, *y, size_on_screen);
                            }

                            *x += size_on_screen;
                        }
                    }
                    ptr_char = utf8_next_char (ptr_char);
                }
                string = utf8_next_char (string);
                break;
        }
    }
    return 1;
}

/*
 * Expands spacers using the sizes computed, replacing them by 0 to N spaces.
 *
 * Note: result must be freed after use.
 */

char *
gui_bar_window_expand_spacers (const char *string, int length_on_screen,
                               int bar_window_width, int num_spacers)
{
    int *spacers, index_spacer, i;
    char **result, *next_char;

    if (!string || !string[0])
        return NULL;

    spacers = gui_bar_window_compute_spacers_size (length_on_screen,
                                                   bar_window_width,
                                                   num_spacers);
    if (!spacers)
        return NULL;

    result = string_dyn_alloc (256);
    if (!result)
    {
        free (spacers);
        return NULL;
    }

    index_spacer = 0;

    while (string && string[0])
    {
        switch (string[0])
        {
            case GUI_COLOR_COLOR_CHAR:
                switch (string[1])
                {
                    case GUI_COLOR_FG_CHAR:
                    case GUI_COLOR_BG_CHAR:
                    case GUI_COLOR_FG_BG_CHAR:
                    case GUI_COLOR_EXTENDED_CHAR:
                    case GUI_COLOR_EMPHASIS_CHAR:
                    case GUI_COLOR_RESET_CHAR:
                        string_dyn_concat (result, string, 2);
                        string += 2;
                        break;
                    case GUI_COLOR_BAR_CHAR:
                        switch (string[2])
                        {
                            case GUI_COLOR_BAR_FG_CHAR:
                            case GUI_COLOR_BAR_DELIM_CHAR:
                            case GUI_COLOR_BAR_BG_CHAR:
                            case GUI_COLOR_BAR_START_INPUT_CHAR:
                            case GUI_COLOR_BAR_START_INPUT_HIDDEN_CHAR:
                            case GUI_COLOR_BAR_MOVE_CURSOR_CHAR:
                            case GUI_COLOR_BAR_START_ITEM:
                            case GUI_COLOR_BAR_START_LINE_ITEM:
                                string_dyn_concat (result, string, 3);
                                string += 3;
                                break;
                            case GUI_COLOR_BAR_SPACER:
                                if (index_spacer < num_spacers)
                                {
                                    for (i = 0; i < spacers[index_spacer]; i++)
                                    {
                                        string_dyn_concat (result, " ", 1);
                                    }
                                }
                                index_spacer++;
                                string += 3;
                                break;
                            default:
                                string_dyn_concat (result, string, 2);
                                string += 2;
                                break;
                        }
                        break;
                    default:
                        string_dyn_concat (result, string, 1);
                        string++;
                        break;
                }
                break;
            case GUI_COLOR_SET_ATTR_CHAR:
            case GUI_COLOR_REMOVE_ATTR_CHAR:
            case GUI_COLOR_RESET_CHAR:
                string_dyn_concat (result, string, 1);
                string++;
                break;
            default:
                next_char = (char *)utf8_next_char (string);
                if (!next_char)
                    break;
                string_dyn_concat (result, string, next_char - string);
                string = next_char;
                break;
        }
    }

    free (spacers);

    return string_dyn_free (result, 0);
}

/*
 * Draws a bar for a window.
 */

void
gui_bar_window_draw (struct t_gui_bar_window *bar_window,
                     struct t_gui_window *window)
{
    int x, y, items_count, num_lines, line, color_bg, bar_position, bar_size;
    enum t_gui_bar_filling bar_filling;
    char *content, *content2, **items;
    static char str_start_input[16] = { '\0' };
    static char str_start_input_hidden[16] = { '\0' };
    static char str_cursor[16] = { '\0' };
    char *pos_start_input, *pos_after_start_input, *pos_cursor, *buf;
    char *new_start_input, *ptr_string;
    static int length_start_input, length_start_input_hidden;
    int num_spacers, length_on_screen;
    int chars_available, index, size;
    int length_screen_before_cursor, length_screen_after_cursor;
    int diff, max_length, optimal_number_of_lines;
    int some_data_not_displayed;
    int index_item, index_subitem, index_line;

    if (!gui_init_ok)
        return;

    if (gui_window_bare_display)
        return;

    if (!bar_window || (bar_window->x < 0) || (bar_window->y < 0))
        return;

    color_bg = ((CONFIG_ENUM(bar_window->bar->options[GUI_BAR_OPTION_TYPE]) != GUI_BAR_TYPE_ROOT)
                && (window != gui_current_window)) ?
        GUI_BAR_OPTION_COLOR_BG_INACTIVE : GUI_BAR_OPTION_COLOR_BG;

    if (!str_start_input[0])
    {
        snprintf (str_start_input, sizeof (str_start_input), "%c%c%c",
                  GUI_COLOR_COLOR_CHAR,
                  GUI_COLOR_BAR_CHAR,
                  GUI_COLOR_BAR_START_INPUT_CHAR);
        length_start_input = strlen (str_start_input);

        snprintf (str_start_input_hidden, sizeof (str_start_input_hidden), "%c%c%c",
                  GUI_COLOR_COLOR_CHAR,
                  GUI_COLOR_BAR_CHAR,
                  GUI_COLOR_BAR_START_INPUT_HIDDEN_CHAR);
        length_start_input_hidden = strlen (str_start_input_hidden);

        snprintf (str_cursor, sizeof (str_cursor), "%c%c%c",
                  GUI_COLOR_COLOR_CHAR,
                  GUI_COLOR_BAR_CHAR,
                  GUI_COLOR_BAR_MOVE_CURSOR_CHAR);
    }

    /*
     * these values will be overwritten later (by gui_bar_window_print_string)
     * if cursor has to move somewhere in bar window
     */
    bar_window->cursor_x = -1;
    bar_window->cursor_y = -1;

    /* remove coords */
    gui_bar_window_coords_free (bar_window);
    index_item = -1;
    index_subitem = -1;
    index_line = 0;

    gui_window_current_emphasis = 0;

    bar_position = CONFIG_ENUM(bar_window->bar->options[GUI_BAR_OPTION_POSITION]);
    bar_filling = gui_bar_get_filling (bar_window->bar);
    bar_size = CONFIG_INTEGER(bar_window->bar->options[GUI_BAR_OPTION_SIZE]);

    content = gui_bar_window_content_get_with_filling (bar_window, window,
                                                       &num_spacers);
    if (content)
    {
        utf8_normalize (content, '?');
        length_on_screen = gui_chat_strlen_screen (content);
        if ((num_spacers > 0) && gui_bar_window_can_use_spacer (bar_window))
        {
            content2 = gui_bar_window_expand_spacers (content,
                                                      length_on_screen,
                                                      bar_window->width,
                                                      num_spacers);
            if (content2)
            {
                free (content);
                content = content2;
            }
        }
        if ((bar_filling == GUI_BAR_FILLING_HORIZONTAL)
            && (bar_window->scroll_x > 0))
        {
            if (bar_window->scroll_x > length_on_screen - bar_window->width)
            {
                bar_window->scroll_x = length_on_screen - bar_window->width;
                if (bar_window->scroll_x < 0)
                    bar_window->scroll_x = 0;
            }
        }

        items = string_split (content, "\n", NULL,
                              WEECHAT_STRING_SPLIT_STRIP_LEFT
                              | WEECHAT_STRING_SPLIT_STRIP_RIGHT,
                              0, &items_count);
        if (items_count == 0)
        {
            if (bar_size == 0)
                gui_bar_window_set_current_size (bar_window, window, 1);
            gui_window_clear (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                              CONFIG_COLOR(bar_window->bar->options[GUI_BAR_OPTION_COLOR_FG]),
                              CONFIG_COLOR(bar_window->bar->options[color_bg]));
        }
        else
        {
            /* bar with auto size ? then compute new size, according to content */
            if (bar_size == 0)
            {
                /* search longer line and optimal number of lines */
                max_length = 0;
                optimal_number_of_lines = 0;
                for (line = 0; line < items_count; line++)
                {
                    length_on_screen = gui_chat_strlen_screen (items[line]);

                    pos_cursor = strstr (items[line], str_cursor);
                    if (pos_cursor && (gui_chat_strlen_screen (pos_cursor) == 0))
                        length_on_screen++;

                    if (length_on_screen > max_length)
                        max_length = length_on_screen;

                    if (length_on_screen % bar_window->width == 0)
                        num_lines = length_on_screen / bar_window->width;
                    else
                        num_lines = (length_on_screen / bar_window->width) + 1;
                    if (num_lines == 0)
                        num_lines = 1;
                    optimal_number_of_lines += num_lines;
                }
                if (max_length == 0)
                    max_length = 1;

                switch (bar_position)
                {
                    case GUI_BAR_POSITION_BOTTOM:
                    case GUI_BAR_POSITION_TOP:
                        if (bar_filling == GUI_BAR_FILLING_HORIZONTAL)
                            num_lines = optimal_number_of_lines;
                        else
                            num_lines = items_count;
                        gui_bar_window_set_current_size (bar_window, window,
                                                         num_lines);
                        break;
                    case GUI_BAR_POSITION_LEFT:
                    case GUI_BAR_POSITION_RIGHT:
                        gui_bar_window_set_current_size (bar_window, window,
                                                         max_length);
                        break;
                    case GUI_BAR_NUM_POSITIONS:
                        break;
                }
            }

            gui_window_clear (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                              CONFIG_COLOR(bar_window->bar->options[GUI_BAR_OPTION_COLOR_FG]),
                              CONFIG_COLOR(bar_window->bar->options[color_bg]));
            x = 0;
            y = 0;
            some_data_not_displayed = 0;
            if ((bar_window->scroll_y > 0)
                && (bar_window->scroll_y > items_count - bar_window->height))
            {
                bar_window->scroll_y = items_count - bar_window->height;
                if (bar_window->scroll_y < 0)
                    bar_window->scroll_y = 0;
            }
            for (line = 0;
                 (line < items_count) && (y < bar_window->height);
                 line++)
            {
                pos_start_input = strstr (items[line], str_start_input);
                if (pos_start_input)
                {
                    pos_after_start_input = pos_start_input + strlen (str_start_input);
                    pos_cursor = strstr (pos_after_start_input, str_cursor);
                    if (pos_cursor)
                    {
                        chars_available =
                            ((bar_window->height - y - 1) * bar_window->width) + /* next lines */
                            (bar_window->width - x - 1); /* chars on current line */

                        length_screen_before_cursor = -1;
                        length_screen_after_cursor = -1;

                        buf = string_strndup (items[line], pos_cursor - items[line]);
                        if (buf)
                        {
                            length_screen_before_cursor = gui_chat_strlen_screen (buf);
                            length_screen_after_cursor = gui_chat_strlen_screen (pos_cursor);
                            free (buf);
                        }

                        if ((length_screen_before_cursor < 0) || (length_screen_after_cursor < 0))
                            length_screen_before_cursor = gui_chat_strlen_screen (items[line]);

                        diff = length_screen_before_cursor - chars_available;
                        if (diff > 0)
                        {
                            if (CONFIG_INTEGER(config_look_input_cursor_scroll) > 0)
                            {
                                diff += (CONFIG_INTEGER(config_look_input_cursor_scroll)
                                         - 1
                                         - (diff % CONFIG_INTEGER(config_look_input_cursor_scroll)));
                            }

                            /* compute new start for displaying input */
                            new_start_input = pos_after_start_input +
                                gui_chat_string_real_pos (pos_after_start_input,
                                                          diff, 1);
                            if (new_start_input > pos_cursor)
                                new_start_input = pos_cursor;

                            buf = malloc (strlen (items[line]) + length_start_input_hidden + 1);
                            if (buf)
                            {
                                /* add string before start of input */
                                index = 0;
                                if (pos_start_input > items[line])
                                {
                                    size = pos_start_input - items[line];
                                    memmove (buf, items[line], size);
                                    index += size;
                                }
                                /* add tag "start_input_hidden" */
                                memmove (buf + index, str_start_input_hidden, length_start_input_hidden);
                                index += length_start_input_hidden;
                                /* add hidden part of input */
                                size = new_start_input - pos_after_start_input;
                                memmove (buf + index, pos_after_start_input, size);
                                index += size;
                                /* add tag "start_input" */
                                memmove (buf + index, str_start_input, length_start_input);
                                index += length_start_input;
                                /* add input (will be displayed) */
                                size = strlen (new_start_input) + 1;
                                memmove (buf + index, new_start_input, size);

                                free (items[line]);
                                items[line] = buf;
                            }
                        }

                    }
                }

                if ((bar_window->scroll_y == 0)
                    || (line >= bar_window->scroll_y))
                {
                    if (!gui_bar_window_print_string (bar_window,
                                                      window,
                                                      bar_filling,
                                                      &x, &y,
                                                      items[line], 1, 1,
                                                      &index_item,
                                                      &index_subitem,
                                                      &index_line))
                    {
                        some_data_not_displayed = 1;
                    }

                    if (x < bar_window->width)
                    {
                        if (bar_filling == GUI_BAR_FILLING_HORIZONTAL)
                        {
                            gui_window_set_custom_color_fg_bg (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                               CONFIG_COLOR(bar_window->bar->options[GUI_BAR_OPTION_COLOR_FG]),
                                                               CONFIG_COLOR(bar_window->bar->options[color_bg]),
                                                               1);
                            gui_window_remove_color_style (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                           A_ALL_ATTR);
                            wclrtobot (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar);
                        }
                        else
                        {
                            gui_window_remove_color_style (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                           A_ALL_ATTR);
                        }
                        while (x < bar_window->width)
                        {
                            gui_bar_window_print_string (bar_window,
                                                         window,
                                                         bar_filling,
                                                         &x, &y, " ", 0, 0,
                                                         &index_item,
                                                         &index_subitem,
                                                         &index_line);
                        }
                    }

                    x = 0;
                    y++;
                }
            }
            if ((bar_window->cursor_x < 0) && (bar_window->cursor_y < 0)
                && ((bar_window->scroll_x > 0) || (bar_window->scroll_y > 0)))
            {
                if (bar_filling == GUI_BAR_FILLING_HORIZONTAL)
                {
                    ptr_string = CONFIG_STRING(config_look_bar_more_left);
                    x = 0;
                }
                else
                {
                    ptr_string = CONFIG_STRING(config_look_bar_more_up);
                    x = bar_window->width - utf8_strlen_screen (ptr_string);
                    if (x < 0)
                        x = 0;
                }
                y = 0;
                if (ptr_string && ptr_string[0])
                {
                    gui_window_set_custom_color_fg_bg (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                       CONFIG_COLOR(config_color_bar_more),
                                                       CONFIG_COLOR(bar_window->bar->options[color_bg]),
                                                       1);
                    mvwaddstr (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                               y, x, ptr_string);
                }
            }
            if ((bar_window->cursor_x < 0) && (bar_window->cursor_y < 0)
                && (some_data_not_displayed || (line < items_count)))
            {
                ptr_string = (bar_filling == GUI_BAR_FILLING_HORIZONTAL) ?
                    CONFIG_STRING(config_look_bar_more_right) :
                    CONFIG_STRING(config_look_bar_more_down);
                x = bar_window->width - utf8_strlen_screen (ptr_string);
                if (x < 0)
                    x = 0;
                y = (bar_window->height > 1) ? bar_window->height - 1 : 0;
                if (ptr_string && ptr_string[0])
                {
                    gui_window_set_custom_color_fg_bg (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                       CONFIG_COLOR(config_color_bar_more),
                                                       CONFIG_COLOR(bar_window->bar->options[color_bg]),
                                                       1);
                    mvwaddstr (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                               y, x, ptr_string);
                }
            }
        }
        string_free_split (items);
        free (content);
    }
    else
    {
        if (bar_size == 0)
            gui_bar_window_set_current_size (bar_window, window, 1);
        gui_window_clear (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                          CONFIG_COLOR(bar_window->bar->options[GUI_BAR_OPTION_COLOR_FG]),
                          CONFIG_COLOR(bar_window->bar->options[color_bg]));
    }

    /*
     * move cursor if it was asked in an item content (input_text does that
     * to move cursor in user input text)
     */
    if ((!window || (gui_current_window == window))
        && (bar_window->cursor_x >= 0) && (bar_window->cursor_y >= 0))
    {
        y = bar_window->cursor_y - bar_window->y;
        x = bar_window->cursor_x - bar_window->x;
        if (x > bar_window->width - 2)
            x = bar_window->width - 2;
        wmove (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar, y, x);
        wrefresh (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar);
        if (!gui_cursor_mode)
        {
            gui_window_cursor_x = bar_window->cursor_x;
            gui_window_cursor_y = bar_window->cursor_y;
            move (bar_window->cursor_y, bar_window->cursor_x);
        }
    }
    else
        wnoutrefresh (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar);

    if (CONFIG_INTEGER(bar_window->bar->options[GUI_BAR_OPTION_SEPARATOR]))
    {
        switch (bar_position)
        {
            case GUI_BAR_POSITION_BOTTOM:
                gui_window_set_weechat_color (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator,
                                              GUI_COLOR_SEPARATOR);
                gui_window_hline (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator,
                                  0, 0, bar_window->width,
                                  CONFIG_STRING(config_look_separator_horizontal));
                break;
            case GUI_BAR_POSITION_TOP:
                gui_window_set_weechat_color (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator,
                                              GUI_COLOR_SEPARATOR);
                gui_window_hline (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator,
                                  0, 0, bar_window->width,
                                  CONFIG_STRING(config_look_separator_horizontal));
                break;
            case GUI_BAR_POSITION_LEFT:
                gui_window_set_weechat_color (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator,
                                              GUI_COLOR_SEPARATOR);
                gui_window_vline (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator,
                                  0, 0, bar_window->height,
                                  CONFIG_STRING(config_look_separator_vertical));
                break;
            case GUI_BAR_POSITION_RIGHT:
                gui_window_set_weechat_color (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator,
                                              GUI_COLOR_SEPARATOR);
                gui_window_vline (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator,
                                  0, 0, bar_window->height,
                                  CONFIG_STRING(config_look_separator_vertical));
                break;
            case GUI_BAR_NUM_POSITIONS:
                break;
        }
        wnoutrefresh (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator);
    }

    refresh ();
}

/*
 * Prints bar window infos in WeeChat log file (usually for crash dump).
 */

void
gui_bar_window_objects_print_log (struct t_gui_bar_window *bar_window)
{
    log_printf ("    bar window specific objects for Curses:");
    log_printf ("      win_bar. . . . . . . : %p", GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar);
    log_printf ("      win_separator. . . . : %p", GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator);
}
