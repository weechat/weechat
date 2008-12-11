/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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

/* gui-curses-bar-window.c: bar window functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-log.h"
#include "../../core/wee-string.h"
#include "../../core/wee-utf8.h"
#include "../gui-bar.h"
#include "../gui-bar-item.h"
#include "../gui-bar-window.h"
#include "../gui-chat.h"
#include "../gui-color.h"
#include "../gui-window.h"
#include "gui-curses.h"


/*
 * gui_bar_window_objects_init: init Curses windows for bar window
 */

int
gui_bar_window_objects_init (struct t_gui_bar_window *bar_window)
{
    struct t_gui_bar_window_curses_objects *new_objects;
    
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
 * gui_window_objects_free: free Curses windows for a bar window
 */

void
gui_bar_window_objects_free (struct t_gui_bar_window *bar_window)
{
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
 * gui_bar_window_create_win: create curses window for bar
 */

void
gui_bar_window_create_win (struct t_gui_bar_window *bar_window)
{
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
    
    GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar = newwin (bar_window->height,
                                                          bar_window->width,
                                                          bar_window->y,
                                                          bar_window->x);
    
    if (CONFIG_INTEGER(bar_window->bar->separator))
    {
        switch (CONFIG_INTEGER(bar_window->bar->position))
        {
            case GUI_BAR_POSITION_BOTTOM:
                GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator = newwin (1,
                                                                            bar_window->width,
                                                                            bar_window->y - 1,
                                                                            bar_window->x);
                break;
            case GUI_BAR_POSITION_TOP:
                GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator = newwin (1,
                                                                            bar_window->width,
                                                                            bar_window->y + bar_window->height,
                                                                            bar_window->x);
                break;
            case GUI_BAR_POSITION_LEFT:
                GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator = newwin (bar_window->height,
                                                                            1,
                                                                            bar_window->y,
                                                                            bar_window->x + bar_window->width);
                break;
            case GUI_BAR_POSITION_RIGHT:
                GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator = newwin (bar_window->height,
                                                                            1,
                                                                            bar_window->y,
                                                                            bar_window->x - 1);
                break;
            case GUI_BAR_NUM_POSITIONS:
                break;
        }
    }
}

/*
 * gui_bar_window_print_string: print a string text on a bar window
 *                              return 1 if all was printed, 0 if some text
 *                              was not displayed (wrapped due to bar window
 *                              width)
 */

int
gui_bar_window_print_string (struct t_gui_bar_window *bar_window,
                             int *x, int *y,
                             const char *string,
                             int reset_color_before_display)
{
    int weechat_color, x_with_hidden, size_on_screen, fg, bg, low_char;
    char str_fg[3], str_bg[3], utf_char[16], *next_char, *output;
    
    if (!string || !string[0])
        return 1;
    
    wmove (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar, *y, *x);

    if (reset_color_before_display)
    {
        gui_window_set_custom_color_fg_bg (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                           CONFIG_COLOR(bar_window->bar->color_fg),
                                           CONFIG_COLOR(bar_window->bar->color_bg));
    }
    
    x_with_hidden = *x;
    
    while (string && string[0])
    {
        if (string[0] == GUI_COLOR_COLOR_CHAR)
        {
            string++;
            switch (string[0])
            {
                case GUI_COLOR_FG_CHAR: /* fg color */
                    if (string[1] && string[2])
                    {
                        str_fg[0] = string[1];
                        str_fg[1] = string[2];
                        str_fg[2] = '\0';
                        sscanf (str_fg, "%d", &fg);
                        gui_window_set_custom_color_fg (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                        fg);
                        string += 3;
                    }
                    break;
                case GUI_COLOR_BG_CHAR: /* bg color */
                    if (string[1] && string[2])
                    {
                        str_bg[0] = string[1];
                        str_bg[1] = string[2];
                        str_bg[2] = '\0';
                        sscanf (str_bg, "%d", &bg);
                        gui_window_set_custom_color_bg (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                        bg);
                        string += 3;
                    }
                    break;
                case GUI_COLOR_FG_BG_CHAR: /* fg + bg color */
                    if (string[1] && string[2] && (string[3] == ',')
                        && string[4] && string[5])
                    {
                        str_fg[0] = string[1];
                        str_fg[1] = string[2];
                        str_fg[2] = '\0';
                        str_bg[0] = string[4];
                        str_bg[1] = string[5];
                        str_bg[2] = '\0';
                        sscanf (str_fg, "%d", &fg);
                        sscanf (str_bg, "%d", &bg);
                        gui_window_set_custom_color_fg_bg (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                           fg, bg);
                        string += 6;
                    }
                    break;
                case GUI_COLOR_BAR_CHAR: /* bar color */
                    switch (string[1])
                    {
                        case GUI_COLOR_BAR_FG_CHAR:
                            /* bar foreground */
                            gui_window_set_custom_color_fg (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                            CONFIG_INTEGER(bar_window->bar->color_fg));
                            string += 2;
                            break;
                        case GUI_COLOR_BAR_DELIM_CHAR:
                            /* bar delimiter */
                            gui_window_set_custom_color_fg (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                            CONFIG_INTEGER(bar_window->bar->color_delim));
                            string += 2;
                            break;
                        case GUI_COLOR_BAR_BG_CHAR:
                            /* bar background */
                            gui_window_set_custom_color_bg (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                            CONFIG_INTEGER(bar_window->bar->color_bg));
                            string += 2;
                            break;
                        case GUI_COLOR_BAR_MOVE_CURSOR_CHAR:
                            /* move cursor to current position on screen */
                            getyx (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                   bar_window->cursor_y, bar_window->cursor_x);
                            bar_window->cursor_x += bar_window->x;
                            bar_window->cursor_y += bar_window->y;
                            string += 2;
                            break;
                        default:
                            string++;
                            break;
                    }
                    break;
                default:
                    if (isdigit (string[0]) && isdigit (string[1]))
                    {
                        str_fg[0] = string[0];
                        str_fg[1] = string[1];
                        str_fg[2] = '\0';
                        sscanf (str_fg, "%d", &weechat_color);
                        gui_window_set_weechat_color (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                      weechat_color);
                        string += 2;
                    }
                    break;
            }
        }
        else
        {
            next_char = utf8_next_char (string);
            if (!next_char)
                break;
            
            memcpy (utf_char, string, next_char - string);
            utf_char[next_char - string] = '\0';
            
            if ((((unsigned char)utf_char[0]) < 32) && (!utf_char[1]))
            {
                low_char = 1;
                snprintf (utf_char, sizeof (utf_char), "%c",
                          'A' + ((unsigned char)utf_char[0]) - 1);
            }
            else
            {
                low_char = 0;
                if (!gui_window_utf_char_valid (utf_char))
                    snprintf (utf_char, sizeof (utf_char), ".");
            }
            
            size_on_screen = utf8_char_size_screen (utf_char);
            if (size_on_screen > 0)
            {
                if (x_with_hidden < bar_window->scroll_x)
                {
                    /* hidden char (before scroll_x value) */
                    x_with_hidden++;
                }
                else
                {
                    if (*x + size_on_screen > bar_window->width)
                    {
                        if (CONFIG_INTEGER(gui_bar_get_option_filling (bar_window->bar)) == GUI_BAR_FILLING_VERTICAL)
                            return 0;
                        if (*y >= bar_window->height - 1)
                            return 0;
                        *x = 0;
                        (*y)++;
                        wmove (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar, *y, *x);
                    }
                    
                    output = string_iconv_from_internal (NULL, utf_char);
                    if (low_char)
                        wattron (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar, A_REVERSE);
                    wprintw (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar, "%s",
                             (output) ? output : utf_char);
                    if (low_char)
                        wattroff (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar, A_REVERSE);
                    if (output)
                        free (output);
                    
                    *x += size_on_screen;
                }
            }
            
            string = next_char;
        }
    }
    return 1;
}

/*
 * gui_bar_window_draw: draw a bar for a window
 */

void
gui_bar_window_draw (struct t_gui_bar_window *bar_window,
                     struct t_gui_window *window)
{
    int x, y, i, items_count, num_lines, line;
    char *content, *item_value, *item_value2, **items;
    char space_with_reinit_color[32];
    int length_reinit_color, content_length, length, length_on_screen;
    int max_length, optimal_number_of_lines, chars_available;
    int some_data_not_displayed;
    
    if (!gui_init_ok)
        return;
    
    snprintf (space_with_reinit_color,
              sizeof (space_with_reinit_color),
              "%c%c%02d,%02d ",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_FG_BG_CHAR,
              CONFIG_COLOR(bar_window->bar->color_fg),
              CONFIG_COLOR(bar_window->bar->color_bg));
    length_reinit_color = strlen (space_with_reinit_color);
    
    /* these values will be overwritten later (by gui_bar_window_print_string)
       if cursor has to move somewhere in bar window */
    bar_window->cursor_x = -1;
    bar_window->cursor_y = -1;
    
    if (CONFIG_INTEGER(bar_window->bar->size) == 0)
    {
        content = NULL;
        content_length = 1;
        for (i = 0; i < bar_window->bar->items_count; i++)
        {
            item_value = gui_bar_item_get_value (bar_window->bar->items_array[i],
                                                 bar_window->bar, window,
                                                 0, 0, 0);
            if (item_value)
            {
                if (item_value[0])
                {
                    if (CONFIG_INTEGER(gui_bar_get_option_filling (bar_window->bar)) == GUI_BAR_FILLING_HORIZONTAL)
                    {
                        item_value2 = string_replace (item_value, "\n",
                                                      space_with_reinit_color);
                    }
                    else
                        item_value2 = NULL;
                    if (!content)
                    {
                        content_length += strlen ((item_value2) ?
                                                  item_value2 : item_value);
                        content = strdup ((item_value2) ?
                                          item_value2 : item_value);
                    }
                    else
                    {
                        content_length += length_reinit_color +
                            strlen ((item_value2) ? item_value2 : item_value);
                        content = realloc (content, content_length);
                        if (CONFIG_INTEGER(gui_bar_get_option_filling (bar_window->bar)) == GUI_BAR_FILLING_HORIZONTAL)
                            strcat (content, space_with_reinit_color);
                        else
                            strcat (content, "\n");
                        strcat (content,
                                (item_value2) ? item_value2 : item_value);
                    }
                    if (item_value2)
                        free (item_value2);
                }
                free (item_value);
            }
        }
        if (content)
        {
            items = string_explode (content, "\n", 0, 0, &items_count);
            if (items_count == 0)
            {
                gui_bar_window_set_current_size (bar_window->bar, 1);
                gui_window_clear (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                  CONFIG_COLOR(bar_window->bar->color_bg));
            }
            else
            {
                /* search longer line and optimal number of lines */
                max_length = 0;
                optimal_number_of_lines = 0;
                for (line = 0; line < items_count; line++)
                {
                    length_on_screen = gui_chat_strlen_screen (items[line]);
                    
                    length = strlen (items[line]);
                    if ((length >= 3)
                        && (items[line][length - 3] == GUI_COLOR_COLOR_CHAR)
                        && (items[line][length - 2] == GUI_COLOR_BAR_CHAR)
                        && (items[line][length - 1] == GUI_COLOR_BAR_MOVE_CURSOR_CHAR))
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
                
                switch (CONFIG_INTEGER(bar_window->bar->position))
                {
                    case GUI_BAR_POSITION_BOTTOM:
                    case GUI_BAR_POSITION_TOP:
                        if (CONFIG_INTEGER(gui_bar_get_option_filling (bar_window->bar)) == GUI_BAR_FILLING_HORIZONTAL)
                            num_lines = optimal_number_of_lines;
                        else
                            num_lines = items_count;
                        gui_bar_window_set_current_size (bar_window->bar,
                                                         num_lines);
                        break;
                    case GUI_BAR_POSITION_LEFT:
                    case GUI_BAR_POSITION_RIGHT:
                        gui_bar_window_set_current_size (bar_window->bar,
                                                         max_length);
                        break;
                    case GUI_BAR_NUM_POSITIONS:
                        break;
                }
                gui_window_clear (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                  CONFIG_COLOR(bar_window->bar->color_bg));
                x = 0;
                y = 0;
                some_data_not_displayed = 0;
                if ((bar_window->scroll_y > 0)
                    && (bar_window->scroll_y >= items_count))
                {
                    bar_window->scroll_y = items_count - bar_window->height;
                    if (bar_window->scroll_y < 0)
                        bar_window->scroll_y = 0;
                }
                for (line = 0;
                     (line < items_count) && (y < bar_window->height);
                     line++)
                {
                    if ((bar_window->scroll_y == 0)
                        || (line >= bar_window->scroll_y))
                    {
                        if (!gui_bar_window_print_string (bar_window, &x, &y,
                                                          items[line], 1))
                        {
                            some_data_not_displayed = 1;
                        }
                        if (CONFIG_INTEGER(gui_bar_get_option_filling (bar_window->bar)) == GUI_BAR_FILLING_VERTICAL)
                        {
                            while (x < bar_window->width)
                            {
                                gui_bar_window_print_string (bar_window,
                                                             &x, &y,
                                                             " ", 0);
                            }
                            x = 0;
                            y++;
                        }
                        else
                        {
                            gui_bar_window_print_string (bar_window, &x, &y,
                                                         space_with_reinit_color, 0);
                        }
                    }
                }
                if ((bar_window->cursor_x < 0) && (bar_window->cursor_y < 0)
                    && ((bar_window->scroll_x > 0) || (bar_window->scroll_y > 0)))
                {
                    x = (bar_window->height > 1) ? bar_window->width - 2 : 0;
                    if (x < 0)
                        x = 0;
                    y = 0;
                    gui_window_set_custom_color_fg_bg (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                       CONFIG_COLOR(config_color_bar_more),
                                                       CONFIG_INTEGER(bar_window->bar->color_bg));
                    mvwprintw (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar, y, x, "--");
                }
                if ((bar_window->cursor_x < 0) && (bar_window->cursor_y < 0)
                    && (some_data_not_displayed || (line < items_count)))
                {
                    x = bar_window->width - 2;
                    if (x < 0)
                        x = 0;
                    y = (bar_window->height > 1) ? bar_window->height - 1 : 0;
                    gui_window_set_custom_color_fg_bg (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                       CONFIG_COLOR(config_color_bar_more),
                                                       CONFIG_INTEGER(bar_window->bar->color_bg));
                    mvwprintw (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar, y, x, "++");
                }
            }
            if (items)
                string_free_exploded (items);
            free (content);
        }
        else
        {
            gui_bar_window_set_current_size (bar_window->bar, 1);
            gui_window_clear (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                              CONFIG_COLOR(bar_window->bar->color_bg));
        }
    }
    else
    {
        gui_window_clear (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                          CONFIG_COLOR(bar_window->bar->color_bg));
        
        x = 0;
        y = 0;
        
        for (i = 0; i < bar_window->bar->items_count; i++)
        {
            chars_available =
                ((bar_window->height - y - 1) * bar_window->width) + /* next lines */
                (bar_window->width - x - 1); /* chars on current line */
            
            item_value = gui_bar_item_get_value (bar_window->bar->items_array[i],
                                                 bar_window->bar, window,
                                                 bar_window->width,
                                                 bar_window->height,
                                                 chars_available);
            if (item_value)
            {
                if (item_value[0])
                {
                    if (CONFIG_INTEGER(gui_bar_get_option_filling (bar_window->bar)) == GUI_BAR_FILLING_HORIZONTAL)
                    {
                        item_value2 = string_replace (item_value, "\n",
                                                      space_with_reinit_color);
                    }
                    else
                        item_value2 = NULL;
                    items = string_explode ((item_value2) ?
                                            item_value2 : item_value,
                                            "\n", 0, 0,
                                            &items_count);
                    some_data_not_displayed = 0;
                    if ((bar_window->scroll_y > 0)
                        && (bar_window->scroll_y >= items_count))
                    {
                        bar_window->scroll_y = items_count - bar_window->height;
                        if (bar_window->scroll_y < 0)
                            bar_window->scroll_y = 0;
                    }
                    for (line = 0;
                         (line < items_count) && (y < bar_window->height);
                         line++)
                    {
                        if ((bar_window->scroll_y == 0)
                            || (line >= bar_window->scroll_y))
                        {
                            if (!gui_bar_window_print_string (bar_window, &x, &y,
                                                              items[line], 1))
                            {
                                some_data_not_displayed = 1;
                            }
                            if (CONFIG_INTEGER(gui_bar_get_option_filling (bar_window->bar)) == GUI_BAR_FILLING_VERTICAL)
                            {
                                while (x < bar_window->width)
                                {
                                    gui_bar_window_print_string (bar_window,
                                                                 &x, &y,
                                                                 " ", 0);
                                }
                                x = 0;
                                y++;
                            }
                            else
                            {
                                gui_bar_window_print_string (bar_window, &x, &y,
                                                             space_with_reinit_color, 0);
                            }
                        }
                    }
                    if ((bar_window->cursor_x < 0) && (bar_window->cursor_y < 0)
                        && ((bar_window->scroll_x > 0) || (bar_window->scroll_y > 0)))
                    {
                        x = (bar_window->height > 1) ? bar_window->width - 2 : 0;
                        if (x < 0)
                            x = 0;
                        y = 0;
                        gui_window_set_custom_color_fg_bg (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                           CONFIG_COLOR(config_color_bar_more),
                                                           CONFIG_INTEGER(bar_window->bar->color_bg));
                        mvwprintw (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar, y, x, "--");
                    }
                    if ((bar_window->cursor_x < 0) && (bar_window->cursor_y < 0)
                        && (some_data_not_displayed || (line < items_count)))
                    {
                        x = bar_window->width - 2;
                        if (x < 0)
                            x = 0;
                        y = (bar_window->height > 1) ? bar_window->height - 1 : 0;
                        gui_window_set_custom_color_fg_bg (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar,
                                                           CONFIG_COLOR(config_color_bar_more),
                                                           CONFIG_INTEGER(bar_window->bar->color_bg));
                        mvwprintw (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar, y, x, "++");
                    }
                    if (item_value2)
                        free (item_value2);
                    if (items)
                        string_free_exploded (items);
                }
                free (item_value);
            }
        }
    }

    /* move cursor if it was asked in an item content (input_text does that
       to move cursor in user input text) */
    if (window && (gui_current_window == window)
        && (bar_window->cursor_x >= 0) && (bar_window->cursor_y >= 0))
    {
        move (bar_window->cursor_y, bar_window->cursor_x);
    }
    
    wnoutrefresh (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar);
    
    if (CONFIG_INTEGER(bar_window->bar->separator))
    {
        switch (CONFIG_INTEGER(bar_window->bar->position))
        {
            case GUI_BAR_POSITION_BOTTOM:
                gui_window_set_weechat_color (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator,
                                              GUI_COLOR_SEPARATOR);
                mvwhline (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator, 0, 0, ACS_HLINE,
                          bar_window->width);
                break;
            case GUI_BAR_POSITION_TOP:
                gui_window_set_weechat_color (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator,
                                              GUI_COLOR_SEPARATOR);
                mvwhline (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator,
                          0, 0, ACS_HLINE, bar_window->width);
                break;
            case GUI_BAR_POSITION_LEFT:
                gui_window_set_weechat_color (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator,
                                              GUI_COLOR_SEPARATOR);
                mvwvline (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator,
                          0, 0, ACS_VLINE, bar_window->height);
                break;
            case GUI_BAR_POSITION_RIGHT:
                gui_window_set_weechat_color (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator,
                                              GUI_COLOR_SEPARATOR);
                mvwvline (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator,
                          0, 0, ACS_VLINE, bar_window->height);
                break;
            case GUI_BAR_NUM_POSITIONS:
                break;
        }
        wnoutrefresh (GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator);
    }
    
    refresh ();
}

/*
 * gui_bar_window_objects_print_log: print bar window infos in log (usually for crash dump)
 */

void
gui_bar_window_objects_print_log (struct t_gui_bar_window *bar_window)
{
    log_printf ("    bar window specific objects for Curses:");
    log_printf ("      win_bar . . . . . . : 0x%lx", GUI_BAR_WINDOW_OBJECTS(bar_window)->win_bar);
    log_printf ("      win_separator . . . : 0x%lx", GUI_BAR_WINDOW_OBJECTS(bar_window)->win_separator);
}
