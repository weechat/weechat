/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

/* gui-curses-input: input display functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-string.h"
#include "../../core/wee-utf8.h"
#include "../../plugins/plugin.h"
#include "../gui-input.h"
#include "../gui-buffer.h"
#include "../gui-color.h"
#include "../gui-keyboard.h"
#include "../gui-main.h"
#include "../gui-window.h"
#include "gui-curses.h"


/*
 * gui_input_set_color: set color for an input window
 */

void
gui_input_set_color (struct t_gui_window *window, int color)
{
    int fg, bg;
    
    fg = gui_weechat_colors[color].foreground;
    bg = gui_color[GUI_COLOR_INPUT]->background;
    
    if (gui_weechat_colors[color].attributes & A_BOLD)
        wattron (GUI_CURSES(window)->win_input, A_BOLD);
    
    if (((fg == -1) || (fg == 99))
        && ((bg == -1) || (bg == 99)))
        wattron (GUI_CURSES(window)->win_input, COLOR_PAIR(63));
    else
    {
        if ((fg == -1) || (fg == 99))
            fg = COLOR_WHITE;
        if ((bg == -1) || (bg == 99))
            bg = 0;
        wattron (GUI_CURSES(window)->win_input, COLOR_PAIR((bg * 8) + fg));
    }
}

/*
 * gui_input_draw_prompt: display input prompt
 *                        return: # chars displayed on screen (one UTF-8 char
 *                        may be displayed on more than 1 char on screen)
 */

void
gui_input_draw_prompt (struct t_gui_window *window)
{
    char *pos, saved_char, *buf;
    int char_size;
    
    wmove (GUI_CURSES(window)->win_input, 0, 0);
    
    if (window->buffer->text_search != GUI_TEXT_SEARCH_DISABLED)
    {
        gui_window_set_weechat_color (GUI_CURSES(window)->win_input,
                                      GUI_COLOR_INPUT);
        wprintw (GUI_CURSES(window)->win_input, "%s",
                 (window->buffer->text_search_exact) ?
                 _("Text search (exact): ") : _("Text search: "));
        return;
    }
    
    pos = CONFIG_STRING(config_look_input_format);
    while (pos && pos[0])
    {
        switch (pos[0])
        {
            case '%':
                pos++;
                switch (pos[0])
                {
                    case 'c': /* buffer name */
                        if (window->buffer->name)
                        {
                            gui_window_set_weechat_color (GUI_CURSES(window)->win_input,
                                                          GUI_COLOR_INPUT_CHANNEL);
                            buf = string_iconv_from_internal (NULL,
                                                              window->buffer->name);
                            wprintw (GUI_CURSES(window)->win_input, "%s",
                                     (buf) ? buf : window->buffer->name);
                            if (buf)
                                free (buf);
                        }
                        pos++;
                        break;
                    case 'm': /* nick modes */
                        /*if (GUI_SERVER(window->buffer) && GUI_SERVER(window->buffer)->is_connected)
                        {
                            if (GUI_SERVER(window->buffer)->nick_modes
                                && GUI_SERVER(window->buffer)->nick_modes[0])
                            {
                                gui_window_set_weechat_color (GUI_CURSES(window)->win_input,
                                                              GUI_COLOR_WIN_INPUT);
                                wprintw (GUI_CURSES(window)->win_input, "%s",
                                         GUI_SERVER(window->buffer)->nick_modes);
                            }
                        }*/
                        pos++;
                        break;
                    case 'n': /* nick */
                        if (window->buffer->input_nick)
                        {
                            gui_window_set_weechat_color (GUI_CURSES(window)->win_input,
                                                          GUI_COLOR_INPUT_NICK);
                            buf = string_iconv_from_internal (NULL,
                                                              window->buffer->input_nick);
                            wprintw (GUI_CURSES(window)->win_input, "%s",
                                     (buf) ? buf : window->buffer->input_nick);
                            if (buf)
                                free (buf);
                        }
                        pos++;
                        break;
                    default:
                        if (pos[0])
                        {
                            char_size = utf8_char_size (pos);
                            saved_char = pos[char_size];
                            pos[char_size] = '\0';
                            gui_window_set_weechat_color (GUI_CURSES(window)->win_input,
                                                          GUI_COLOR_INPUT_DELIMITERS);
                            wprintw (GUI_CURSES(window)->win_input, "%%%s", pos);
                            pos[char_size] = saved_char;
                            pos += char_size;
                        }
                        else
                        {
                            wprintw (GUI_CURSES(window)->win_input, "%%");
                            pos++;
                        }
                        break;
                }
                break;
            default:
                char_size = utf8_char_size (pos);
                saved_char = pos[char_size];
                pos[char_size] = '\0';
                gui_window_set_weechat_color (GUI_CURSES(window)->win_input,
                                              GUI_COLOR_INPUT_DELIMITERS);
                wprintw (GUI_CURSES(window)->win_input, "%s", pos);
                pos[char_size] = saved_char;
                pos += char_size;
                break;
        }
    }
}

/*
 * gui_input_draw_text: display text in input buffer, according to color mask
 *                      return: offset for cursor position on screen (one UTF-8
 *                      char may be displayed on more than 1 char on screen)
 */

int
gui_input_draw_text (struct t_gui_window *window, int input_width)
{
    char *ptr_start, *ptr_next, saved_char, *output, *ptr_string;
    int pos_mask, size, last_color, color, count_cursor, offset_cursor;
    
    ptr_start = utf8_add_offset (window->buffer->input_buffer,
                                 window->buffer->input_buffer_1st_display);
    pos_mask = ptr_start - window->buffer->input_buffer;
    last_color = -1;
    count_cursor = window->buffer->input_buffer_pos -
        window->buffer->input_buffer_1st_display;
    offset_cursor = 0;
    if (window->buffer->text_search != GUI_TEXT_SEARCH_DISABLED)
    {
        if (window->buffer->text_search_found)
            gui_window_set_weechat_color (GUI_CURSES(window)->win_input,
                                          GUI_COLOR_INPUT);
        else
            gui_window_set_weechat_color (GUI_CURSES(window)->win_input,
                                          GUI_COLOR_INPUT_TEXT_NOT_FOUND);
    }
    while ((input_width > 0) && ptr_start && ptr_start[0])
    {
        ptr_next = utf8_next_char (ptr_start);
        if (ptr_next)
        {
            saved_char = ptr_next[0];
            ptr_next[0] = '\0';
            size = ptr_next - ptr_start;
            if (window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
            {
                if (window->buffer->input_buffer_color_mask[pos_mask] != ' ')
                    color = window->buffer->input_buffer_color_mask[pos_mask] - '0';
                else
                    color = -1;
                if (color != last_color)
                {
                    if (color == -1)
                        gui_window_set_weechat_color (GUI_CURSES(window)->win_input,
                                                      GUI_COLOR_INPUT);
                    else
                        gui_input_set_color (window, color);
                }
                last_color = color;
            }
            output = string_iconv_from_internal (NULL, ptr_start);
            
            ptr_string = (output) ? output : ptr_start;
            
            if ((((unsigned char)ptr_string[0]) < 32) && (!ptr_string[1]))
            {
                wattron (GUI_CURSES(window)->win_input, A_REVERSE);
                wprintw (GUI_CURSES(window)->win_input, "%c",
                         'A' + ((unsigned char)ptr_string[0]) - 1);
                wattroff (GUI_CURSES(window)->win_input, A_REVERSE);
                if (count_cursor > 0)
                {
                    offset_cursor++;
                    count_cursor--;
                }
            }
            else
            {
                wprintw (GUI_CURSES(window)->win_input, "%s", ptr_string);
                if (count_cursor > 0)
                {
                    offset_cursor += utf8_strlen_screen (ptr_start);
                    count_cursor--;
                }
            }
            
            if (output)
                free (output);
            
            ptr_next[0] = saved_char;
            ptr_start = ptr_next;
            pos_mask += size;
        }
        else
            ptr_start = NULL;
        input_width--;
    }
    return offset_cursor;
}

/*
 * gui_input_draw: draw input window for a buffer
 */

void
gui_input_draw (struct t_gui_buffer *buffer, int erase)
{
    struct t_gui_window *ptr_win;
    char format[32];
    int prompt_length, display_prompt, offset_cursor;
    /*t_irc_dcc *dcc_selected;*/
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            if (erase)
                gui_window_curses_clear (GUI_CURSES(ptr_win)->win_input,
                                         GUI_COLOR_INPUT);

            if (gui_keyboard_paste_pending)
            {
                wmove (GUI_CURSES(ptr_win)->win_input, 0, 0);
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_input,
                                              GUI_COLOR_INPUT_ACTIONS);
                gui_window_wprintw (GUI_CURSES(ptr_win)->win_input,
                                    _("  Paste %d lines ? [ctrl-Y] Yes  "
                                      "[ctrl-N] No"),
                                    gui_keyboard_get_paste_lines ());
                wclrtoeol (GUI_CURSES(ptr_win)->win_input);
                ptr_win->win_input_cursor_x = 0;
                if (ptr_win == gui_current_window)
                    move (ptr_win->win_input_y, ptr_win->win_input_x);
            }
            else
            {
                if (buffer->input)
                {
                    if (buffer->input_buffer_length == 0)
                        buffer->input_buffer[0] = '\0';
                    
                    prompt_length = gui_input_get_prompt_length (ptr_win->buffer);
                    
                    if (ptr_win->win_input_width - prompt_length < 3)
                    {
                        prompt_length = 0;
                        display_prompt = 0;
                    }
                    else
                        display_prompt = 1;
                    
                    if (buffer->input_buffer_pos - buffer->input_buffer_1st_display + 1 >
                        ptr_win->win_input_width - prompt_length)
                        buffer->input_buffer_1st_display = buffer->input_buffer_pos -
                            (ptr_win->win_input_width - prompt_length) + 1;
                    else
                    {
                        if (buffer->input_buffer_pos < buffer->input_buffer_1st_display)
                            buffer->input_buffer_1st_display = buffer->input_buffer_pos;
                        else
                        {
                            if ((buffer->input_buffer_1st_display > 0) &&
                                (buffer->input_buffer_pos -
                                 buffer->input_buffer_1st_display + 1)
                                < ptr_win->win_input_width - prompt_length)
                            {
                                buffer->input_buffer_1st_display =
                                    buffer->input_buffer_pos -
                                    (ptr_win->win_input_width - prompt_length) + 1;
                                if (buffer->input_buffer_1st_display < 0)
                                    buffer->input_buffer_1st_display = 0;
                            }
                        }
                    }
                    if (display_prompt)
                        gui_input_draw_prompt (ptr_win);
                    
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_input,
                                                  GUI_COLOR_INPUT);
                    snprintf (format, 32, "%%-%ds",
                              ptr_win->win_input_width - prompt_length);
                    offset_cursor = 0;
                    if (ptr_win == gui_current_window)
                        offset_cursor = gui_input_draw_text (ptr_win,
                                                             ptr_win->win_input_width - prompt_length);
                    else
                        wprintw (GUI_CURSES(ptr_win)->win_input, format, "");
                    wclrtoeol (GUI_CURSES(ptr_win)->win_input);
                    ptr_win->win_input_cursor_x = prompt_length + offset_cursor;
                    if (ptr_win == gui_current_window)
                        move (ptr_win->win_input_y,
                              ptr_win->win_input_x + ptr_win->win_input_cursor_x);
                }
                /*
                    case GUI_BUFFER_TYPE_DCC:
                        dcc_selected = (ptr_win->dcc_selected) ? (t_irc_dcc *) ptr_win->dcc_selected : irc_dcc_list;
                        wmove (GUI_CURSES(ptr_win)->win_input, 0, 0);
                        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_input,
                                                      GUI_COLOR_WIN_INPUT_ACTIONS);
                        if (dcc_selected)
                        {
                            switch (dcc_selected->status)
                            {
                                case IRC_DCC_WAITING:
                                    if (IRC_DCC_IS_RECV(dcc_selected->type))
                                        gui_window_wprintw (GUI_CURSES(ptr_win)->win_input,
                                                            _("  [A] Accept"));
                                    gui_window_wprintw (GUI_CURSES(ptr_win)->win_input,
                                                        _("  [C] Cancel"));
                                    break;
                                case IRC_DCC_CONNECTING:
                                case IRC_DCC_ACTIVE:
                                    gui_window_wprintw (GUI_CURSES(ptr_win)->win_input,
                                                        _("  [C] Cancel"));
                                    break;
                                case IRC_DCC_DONE:
                                case IRC_DCC_FAILED:
                                case IRC_DCC_ABORTED:
                                    gui_window_wprintw (GUI_CURSES(ptr_win)->win_input,
                                                        _("  [R] Remove"));
                                    break;
                            }
                        }
                        gui_window_wprintw (GUI_CURSES(ptr_win)->win_input,
                                            _("  [P] Purge old DCC"));
                        gui_window_wprintw (GUI_CURSES(ptr_win)->win_input,
                                            _("  [Q] Close DCC view"));
                        wclrtoeol (GUI_CURSES(ptr_win)->win_input);
                        ptr_win->win_input_cursor_x = 0;
                        if (ptr_win == gui_current_window)
                            move (ptr_win->win_input_y, ptr_win->win_input_x);
                        break;
                    case GUI_BUFFER_TYPE_RAW_DATA:
                        wmove (GUI_CURSES(ptr_win)->win_input, 0, 0);
                        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_input,
                                                      GUI_COLOR_WIN_INPUT_ACTIONS);
                        gui_window_wprintw (GUI_CURSES(ptr_win)->win_input,
                                            _("  [C] Clear buffer"));
                        gui_window_wprintw (GUI_CURSES(ptr_win)->win_input,
                                            _("  [Q] Close raw data view"));
                        wclrtoeol (GUI_CURSES(ptr_win)->win_input);
                        ptr_win->win_input_cursor_x = 0;
                        if (ptr_win == gui_current_window)
                            move (ptr_win->win_input_y, ptr_win->win_input_x);
                        break;
                }*/
            }
            wrefresh (GUI_CURSES(ptr_win)->win_input);
            refresh ();
        }
    }
}
