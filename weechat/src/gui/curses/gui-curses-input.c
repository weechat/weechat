/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

/* gui-curses-input: input display functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../common/utf8.h"
#include "../../common/util.h"
#include "../../common/weeconfig.h"
#include "gui-curses.h"

#ifdef PLUGINS
#include "../../plugins/plugins.h"
#endif


/*
 * gui_input_set_color: set color for an input window
 */

void
gui_input_set_color (t_gui_window *window, int irc_color)
{
    int fg, bg;
    
    fg = gui_irc_colors[irc_color][0];
    bg = gui_color[COLOR_WIN_INPUT]->background;
    
    irc_color %= GUI_NUM_IRC_COLORS;
    if (gui_irc_colors[irc_color][1] & A_BOLD)
        wattron (GUI_CURSES(window)->win_input, A_BOLD);
    
    if (((fg == -1) || (fg == 99))
        && ((bg == -1) || (bg == 99)))
        wattron (GUI_CURSES(window)->win_input, COLOR_PAIR(63));
    else
    {
        if ((fg == -1) || (fg == 99))
            fg = WEECHAT_COLOR_WHITE;
        if ((bg == -1) || (bg == 99))
            bg = 0;
        wattron (GUI_CURSES(window)->win_input, COLOR_PAIR((bg * 8) + fg));
    }
}

/*
 * gui_input_get_prompt_length: return input prompt length (displayed on screen)
 */

int
gui_input_get_prompt_length (t_gui_window *window, char *nick)
{
    char *pos, saved_char;
    int char_size, length;
    
    if (window->buffer->text_search != TEXT_SEARCH_DISABLED)
    {
        if (window->buffer->text_search_exact)
            return utf8_width_screen (_("Text search (exact): "));
        else
            return utf8_width_screen (_("Text search: "));
    }
    
    length = 0;
    pos = cfg_look_input_format;
    while (pos && pos[0])
    {
        switch (pos[0])
        {
            case '%':
                pos++;
                switch (pos[0])
                {
                    case 'c':
                        if (CHANNEL(window->buffer))
                            length += utf8_width_screen (CHANNEL(window->buffer)->name);
                        else
                        {
                            if (SERVER(window->buffer))
                                length += utf8_width_screen (SERVER(window->buffer)->name);
                        }
                        pos++;
                        break;
                    case 'm':
                        if (SERVER(window->buffer))
                        {
                            if (SERVER(window->buffer)->nick_modes
                                && SERVER(window->buffer)->nick_modes[0])
                                length += strlen (SERVER(window->buffer)->nick_modes);
                        }
                        pos++;
                        break;
                    case 'n':
                        length += utf8_width_screen (nick);
                        pos++;
                        break;
                    default:
                        length++;
                        if (pos[0])
                        {
                            if (pos[0] == '%')
                                pos++;
                            else
                            {
                                length++;
                                pos += utf8_char_size (pos);
                            }
                        }
                        break;
                }
                break;
            default:
                char_size = utf8_char_size (pos);
                saved_char = pos[char_size];
                pos[char_size] = '\0';
                length += utf8_width_screen (pos);
                pos[char_size] = saved_char;
                pos += char_size;
                break;
        }
    }
    return length;
}

/*
 * gui_input_draw_prompt: display input prompt
 *                        return: # chars displayed on screen (one UTF-8 char
 *                        may be displayed on more than 1 char on screen)
 */

void
gui_input_draw_prompt (t_gui_window *window, char *nick)
{
    char *pos, saved_char, *buf;
    int char_size;
    
    wmove (GUI_CURSES(window)->win_input, 0, 0);
    
    if (window->buffer->text_search != TEXT_SEARCH_DISABLED)
    {
        gui_window_set_weechat_color (GUI_CURSES(window)->win_input,
                                      COLOR_WIN_INPUT);
        wprintw (GUI_CURSES(window)->win_input, "%s",
                 (window->buffer->text_search_exact) ?
                 _("Text search (exact): ") : _("Text search: "));
        return;
    }
    
    pos = cfg_look_input_format;
    while (pos && pos[0])
    {
        switch (pos[0])
        {
            case '%':
                pos++;
                switch (pos[0])
                {
                    case 'c':
                        if (CHANNEL(window->buffer))
                        {
                            gui_window_set_weechat_color (GUI_CURSES(window)->win_input,
                                                          COLOR_WIN_INPUT_CHANNEL);
                            buf = weechat_iconv_from_internal (NULL,
                                                               CHANNEL(window->buffer)->name);
                            wprintw (GUI_CURSES(window)->win_input, "%s",
                                     (buf) ? buf : CHANNEL(window->buffer)->name);
                            if (buf)
                                free (buf);
                        }
                        else
                        {
                            if (SERVER(window->buffer))
                            {
                                gui_window_set_weechat_color (GUI_CURSES(window)->win_input,
                                                              COLOR_WIN_INPUT_SERVER);
                                buf = weechat_iconv_from_internal (NULL,
                                                                   SERVER(window->buffer)->name);
                                wprintw (GUI_CURSES(window)->win_input, "%s",
                                         (buf) ? buf : SERVER(window->buffer)->name);
                                if (buf)
                                    free (buf);
                            }
                        }
                        pos++;
                        break;
                    case 'm':
                        if (SERVER(window->buffer))
                        {
                            if (SERVER(window->buffer)->nick_modes
                                && SERVER(window->buffer)->nick_modes[0])
                            {
                                gui_window_set_weechat_color (GUI_CURSES(window)->win_input,
                                                              COLOR_WIN_INPUT);
                                wprintw (GUI_CURSES(window)->win_input, "%s",
                                         SERVER(window->buffer)->nick_modes);
                            }
                        }
                        pos++;
                        break;
                    case 'n':
                        gui_window_set_weechat_color (GUI_CURSES(window)->win_input,
                                                      COLOR_WIN_INPUT_NICK);
                        buf = weechat_iconv_from_internal (NULL, nick);
                        wprintw (GUI_CURSES(window)->win_input, "%s", (buf) ? buf : nick);
                        if (buf)
                            free (buf);
                        pos++;
                        break;
                    default:
                        if (pos[0])
                        {
                            char_size = utf8_char_size (pos);
                            saved_char = pos[char_size];
                            pos[char_size] = '\0';
                            gui_window_set_weechat_color (GUI_CURSES(window)->win_input,
                                                          COLOR_WIN_INPUT_DELIMITERS);
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
                                              COLOR_WIN_INPUT_DELIMITERS);
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
gui_input_draw_text (t_gui_window *window, int input_width)
{
    char *ptr_start, *ptr_next, saved_char, *output;
    int pos_mask, size, last_color, color, count_cursor, offset_cursor;
    
    ptr_start = utf8_add_offset (window->buffer->input_buffer,
                                 window->buffer->input_buffer_1st_display);
    pos_mask = ptr_start - window->buffer->input_buffer;
    last_color = -1;
    count_cursor = window->buffer->input_buffer_pos -
        window->buffer->input_buffer_1st_display;
    offset_cursor = 0;
    while ((input_width > 0) && ptr_start && ptr_start[0])
    {
        ptr_next = utf8_next_char (ptr_start);
        if (ptr_next)
        {
            saved_char = ptr_next[0];
            ptr_next[0] = '\0';
            size = ptr_next - ptr_start;
            if (window->buffer->input_buffer_color_mask[pos_mask] != ' ')
                color = window->buffer->input_buffer_color_mask[pos_mask] - '0';
            else
                color = -1;
            if (color != last_color)
            {
                if (color == -1)
                    gui_window_set_weechat_color (GUI_CURSES(window)->win_input, COLOR_WIN_INPUT);
                else
                    gui_input_set_color (window, color);
            }
            last_color = color;
            output = weechat_iconv_from_internal (NULL, ptr_start);
            wprintw (GUI_CURSES(window)->win_input, "%s", (output) ? output : ptr_start);
            if (output)
                free (output);
            if (count_cursor > 0)
            {
                offset_cursor += utf8_width_screen (ptr_start);
                count_cursor--;
            }
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
gui_input_draw (t_gui_buffer *buffer, int erase)
{
    t_gui_window *ptr_win;
    char format[32];
    char *ptr_nickname;
    int prompt_length, display_prompt, offset_cursor;
    t_irc_dcc *dcc_selected;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            if (erase)
                gui_window_curses_clear (GUI_CURSES(ptr_win)->win_input, COLOR_WIN_INPUT);
            
            switch (buffer->type)
            {
                case BUFFER_TYPE_STANDARD:
                    if (buffer->has_input)
                    {
                        if (buffer->input_buffer_length == 0)
                            buffer->input_buffer[0] = '\0';
                        
                        if (SERVER(buffer))
                            ptr_nickname = (SERVER(buffer)->nick) ?
                                SERVER(buffer)->nick : SERVER(buffer)->nick1;
                        else
                            ptr_nickname = cfg_look_no_nickname;
                        
                        prompt_length = gui_input_get_prompt_length (ptr_win, ptr_nickname);
                        
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
                            gui_input_draw_prompt (ptr_win, ptr_nickname);
                        
                        gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_input, COLOR_WIN_INPUT);
                        snprintf (format, 32, "%%-%ds", ptr_win->win_input_width - prompt_length);
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
                    break;
                case BUFFER_TYPE_DCC:
                    dcc_selected = (ptr_win->dcc_selected) ? (t_irc_dcc *) ptr_win->dcc_selected : dcc_list;
                    wmove (GUI_CURSES(ptr_win)->win_input, 0, 0);
                    if (dcc_selected)
                    {
                        switch (dcc_selected->status)
                        {
                            case DCC_WAITING:
                                if (DCC_IS_RECV(dcc_selected->type))
                                    gui_window_wprintw (GUI_CURSES(ptr_win)->win_input,
                                                        _("  [A] Accept"));
                                gui_window_wprintw (GUI_CURSES(ptr_win)->win_input,
                                                    _("  [C] Cancel"));
                                break;
                            case DCC_CONNECTING:
                            case DCC_ACTIVE:
                                gui_window_wprintw (GUI_CURSES(ptr_win)->win_input,
                                                    _("  [C] Cancel"));
                                break;
                            case DCC_DONE:
                            case DCC_FAILED:
                            case DCC_ABORTED:
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
                case BUFFER_TYPE_RAW_DATA:
                    wmove (GUI_CURSES(ptr_win)->win_input, 0, 0);
                    gui_window_wprintw (GUI_CURSES(ptr_win)->win_input,
                                        _("  [C] Clear buffer"));
                    gui_window_wprintw (GUI_CURSES(ptr_win)->win_input,
                                        _("  [Q] Close raw data view"));
                    wclrtoeol (GUI_CURSES(ptr_win)->win_input);
                    ptr_win->win_input_cursor_x = 0;
                    if (ptr_win == gui_current_window)
                        move (ptr_win->win_input_y, ptr_win->win_input_x);
                    break;
            }
            wrefresh (GUI_CURSES(ptr_win)->win_input);
            refresh ();
        }
    }
}
