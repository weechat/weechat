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

/* gui-curses-chat.c: chat display functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-hook.h"
#include "../../core/wee-string.h"
#include "../../core/wee-utf8.h"
#include "../../plugins/plugin.h"
#include "../gui-buffer.h"
#include "../gui-chat.h"
#include "../gui-color.h"
#include "../gui-main.h"
#include "../gui-window.h"
#include "gui-curses.h"


/*
 * gui_chat_set_style: set style (bold, underline, ..)
 *                     for a chat window
 */

void
gui_chat_set_style (struct t_gui_window *window, int style)
{
    wattron (GUI_CURSES(window)->win_chat, style);
}

/*
 * gui_chat_remove_style: remove style (bold, underline, ..)
 *                        for a chat window
 */

void
gui_chat_remove_style (struct t_gui_window *window, int style)
{
    wattroff (GUI_CURSES(window)->win_chat, style);
}

/*
 * gui_chat_toggle_style: toggle a style (bold, underline, ..)
 *                        for a chat window
 */

void
gui_chat_toggle_style (struct t_gui_window *window, int style)
{
    GUI_CURSES(window)->current_style_attr ^= style;
    if (GUI_CURSES(window)->current_style_attr & style)
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
    GUI_CURSES(window)->current_style_fg = -1;
    GUI_CURSES(window)->current_style_bg = -1;
    GUI_CURSES(window)->current_style_attr = 0;
    GUI_CURSES(window)->current_color_attr = 0;
    
    gui_window_set_weechat_color (GUI_CURSES(window)->win_chat, GUI_COLOR_CHAT);
    gui_chat_remove_style (window,
                           A_BOLD | A_UNDERLINE | A_REVERSE);
}

/*
 * gui_chat_set_color_style: set style for color
 */

void
gui_chat_set_color_style (struct t_gui_window *window, int style)
{
    GUI_CURSES(window)->current_color_attr |= style;
    wattron (GUI_CURSES(window)->win_chat, style);
}

/*
 * gui_chat_remove_color_style: remove style for color
 */

void
gui_chat_remove_color_style (struct t_gui_window *window, int style)
{
    GUI_CURSES(window)->current_color_attr &= !style;
    wattroff (GUI_CURSES(window)->win_chat, style);
}

/*
 * gui_chat_reset_color_style: reset style for color
 */

void
gui_chat_reset_color_style (struct t_gui_window *window)
{
    wattroff (GUI_CURSES(window)->win_chat,
              GUI_CURSES(window)->current_color_attr);
    GUI_CURSES(window)->current_color_attr = 0;
}

/*
 * gui_chat_set_color: set color for a chat window
 */

void
gui_chat_set_color (struct t_gui_window *window, int fg, int bg)
{
    GUI_CURSES(window)->current_style_fg = fg;
    GUI_CURSES(window)->current_style_bg = bg;
    
    if (((fg == -1) || (fg == 99))
        && ((bg == -1) || (bg == 99)))
        wattron (GUI_CURSES(window)->win_chat, COLOR_PAIR(63));
    else
    {
        if ((fg == -1) || (fg == 99))
            fg = COLOR_WHITE;
        if ((bg == -1) || (bg == 99))
            bg = 0;
        wattron (GUI_CURSES(window)->win_chat, COLOR_PAIR((bg * 8) + fg));
    }
}

/*
 * gui_chat_set_weechat_color: set a WeeChat color for a chat window
 */

void
gui_chat_set_weechat_color (struct t_gui_window *window, int weechat_color)
{
    if ((weechat_color >= 0) && (weechat_color < GUI_COLOR_NUM_COLORS))
    {
        gui_chat_reset_style (window);
        gui_chat_set_style (window,
                            gui_color[weechat_color]->attributes);
        gui_chat_set_color (window,
                            gui_color[weechat_color]->foreground,
                            gui_color[weechat_color]->background);
    }
}

/*
 * gui_chat_set_custom_color_fg_bg: set a custom color for a chat window
 *                                  (foreground and background)
 */

void
gui_chat_set_custom_color_fg_bg (struct t_gui_window *window, int fg, int bg)
{
    if ((fg >= 0) && (fg < GUI_CURSES_NUM_WEECHAT_COLORS)
        && (bg >= 0) && (bg < GUI_CURSES_NUM_WEECHAT_COLORS))
    {
        gui_chat_reset_style (window);
        gui_chat_set_style (window,
                            gui_weechat_colors[fg].attributes);
        gui_chat_set_color (window,
                            gui_weechat_colors[fg].foreground,
                            gui_weechat_colors[bg].foreground);
    }
}

/*
 * gui_chat_set_custom_color_fg: set a custom color for a chat window
 *                               (foreground only)
 */

void
gui_chat_set_custom_color_fg (struct t_gui_window *window, int fg)
{
    int current_attr, current_bg;
    
    if ((fg >= 0) && (fg < GUI_CURSES_NUM_WEECHAT_COLORS))
    {
        current_attr = GUI_CURSES(window)->current_style_attr;
        current_bg = GUI_CURSES(window)->current_style_bg;
        gui_chat_reset_style (window);
        gui_chat_set_color_style (window, current_attr);
        gui_chat_remove_color_style (window, A_BOLD);
        gui_chat_set_color_style (window, gui_weechat_colors[fg].attributes);
        gui_chat_set_color (window,
                            gui_weechat_colors[fg].foreground,
                            current_bg);
    }
}

/*
 * gui_chat_set_custom_color_bg: set a custom color for a chat window
 *                               (background only)
 */

void
gui_chat_set_custom_color_bg (struct t_gui_window *window, int bg)
{
    int current_attr, current_fg;
    
    if ((bg >= 0) && (bg < GUI_CURSES_NUM_WEECHAT_COLORS))
    {
        current_attr = GUI_CURSES(window)->current_style_attr;
        current_fg = GUI_CURSES(window)->current_style_fg;
        gui_chat_reset_style (window);
        gui_chat_set_color_style (window, current_attr);
        gui_chat_set_color (window,
                            current_fg,
                            gui_weechat_colors[bg].foreground);
    }
}

/*
 * gui_chat_draw_title: draw title window for a buffer
 */

void
gui_chat_draw_title (struct t_gui_buffer *buffer, int erase)
{
    struct t_gui_window *ptr_win;
    char format[32], *buf, *title_decoded, *ptr_title;
    
    if (!gui_ok)
        return;
    
    title_decoded = (buffer->title) ?
        (char *)gui_color_decode ((unsigned char *)buffer->title) : NULL;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            if (erase)
                gui_window_curses_clear (GUI_CURSES(ptr_win)->win_title, GUI_COLOR_TITLE);
            
            snprintf (format, sizeof (format), "%%-%ds", ptr_win->win_title_width);
            wmove (GUI_CURSES(ptr_win)->win_title, 0, 0);
            
            if (title_decoded)
            {
                ptr_title = utf8_add_offset (title_decoded,
                                             ptr_win->win_title_start);
                if (!ptr_title || !ptr_title[0])
                {
                    ptr_win->win_title_start = 0;
                    ptr_title = title_decoded;
                }
                buf = string_iconv_from_internal (NULL,
                                                  ptr_title);
                if (buf)
                    ptr_title = buf;
                
                if (ptr_win->win_title_start > 0)
                {
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_title,
                                                  GUI_COLOR_TITLE_MORE);
                    wprintw (GUI_CURSES(ptr_win)->win_title, "%s", "++");
                }
                
                if (utf8_strlen_screen (ptr_title) > ptr_win->win_width)
                {
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_title,
                                                  GUI_COLOR_TITLE);
                    wprintw (GUI_CURSES(ptr_win)->win_title, "%s", ptr_title);
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_title,
                                                  GUI_COLOR_TITLE_MORE);
                    mvwprintw (GUI_CURSES(ptr_win)->win_title, 0,
                               ptr_win->win_width - 2,
                               "%s", "++");
                }
                else
                {
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_title,
                                                  GUI_COLOR_TITLE);
                    wprintw (GUI_CURSES(ptr_win)->win_title, "%s", ptr_title);
                }
                if (buf)
                    free (buf);
            }
            else
            {
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_title,
                                              GUI_COLOR_TITLE);
                wprintw (GUI_CURSES(ptr_win)->win_title, format, " ");
            }
            wnoutrefresh (GUI_CURSES(ptr_win)->win_title);
            refresh ();
        }
    }
    
    if (title_decoded)
        free (title_decoded);
}

/*
 * gui_chat_get_real_width: return real width: width - 1 if nicklist is at right,
 *                          for good copy/paste (without nicklist separator)
 */

int
gui_chat_get_real_width (struct t_gui_window *window)
{
    if (window->buffer->nicklist
        && (CONFIG_INTEGER(config_look_nicklist_position) == CONFIG_LOOK_NICKLIST_RIGHT))
        return window->win_chat_width - 1;
    else
        return window->win_chat_width;
}

/*
 * gui_chat_display_new_line: display a new line
 */

void
gui_chat_display_new_line (struct t_gui_window *window, int num_lines, int count,
                           int *lines_displayed, int simulate)
{
    if ((count == 0) || (*lines_displayed >= num_lines - count))
    {
        if ((!simulate)
            && (window->win_chat_cursor_x <= gui_chat_get_real_width (window) - 1))
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
 * gui_chat_string_next_char: returns next char of a word (for display)
 *                            special chars like colors, bold, .. are skipped
 *                            and optionaly applied
 */

char *
gui_chat_string_next_char (struct t_gui_window *window, unsigned char *string,
                           int apply_style)
{
    char str_fg[3], str_bg[3];
    int weechat_color, fg, bg;
    
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
                switch (string[0])
                {
                    case 'F':
                        if (string[1] && string[2])
                        {
                            if (apply_style)
                            {
                                str_fg[0] = string[1];
                                str_fg[1] = string[2];
                                str_fg[2] = '\0';
                                sscanf (str_fg, "%d", &fg);
                                gui_chat_set_custom_color_fg (window, fg);
                            }
                            string += 3;
                        }
                        break;
                    case 'B':
                        if (string[1] && string[2])
                        {
                            if (apply_style)
                            {
                                str_bg[0] = string[1];
                                str_bg[1] = string[2];
                                str_bg[2] = '\0';
                                sscanf (str_bg, "%d", &bg);
                                gui_chat_set_custom_color_bg (window, bg);
                            }
                            string += 3;
                        }
                        break;
                    case '*':
                        if (string[1] && string[2] && (string[3] == ',')
                            && string[4] && string[5])
                        {
                            if (apply_style)
                            {
                                str_fg[0] = string[1];
                                str_fg[1] = string[2];
                                str_fg[2] = '\0';
                                str_bg[0] = string[4];
                                str_bg[1] = string[5];
                                str_bg[2] = '\0';
                                sscanf (str_fg, "%d", &fg);
                                sscanf (str_bg, "%d", &bg);
                                gui_chat_set_custom_color_fg_bg (window, fg, bg);
                            }
                            string += 6;
                        }
                        break;
                    default:
                        if (isdigit (string[0]) && isdigit (string[1]))
                        {
                            if (apply_style)
                            {
                                str_fg[0] = string[0];
                                str_fg[1] = string[1];
                                str_fg[2] = '\0';
                                sscanf (str_fg, "%d", &weechat_color);
                                gui_chat_set_weechat_color (window, weechat_color);
                            }
                            string += 2;
                        }
                        break;
                }
                break;
            case GUI_COLOR_SET_CHAR:
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
            case GUI_COLOR_REMOVE_CHAR:
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
gui_chat_display_word_raw (struct t_gui_window *window, char *string,
                           int max_chars_on_screen, int display)
{
    char *next_char, *output, utf_char[16], chars_displayed, size_screen;
    
    if (display)
        wmove (GUI_CURSES(window)->win_chat,
               window->win_chat_cursor_y,
               window->win_chat_cursor_x);
    
    chars_displayed = 0;
    
    while (string && string[0])
    {
        string = gui_chat_string_next_char (window,
                                            (unsigned char *)string, 1);
        if (!string)
            return;
        
        next_char = utf8_next_char (string);
        if (display && next_char)
        {
            memcpy (utf_char, string, next_char - string);
            utf_char[next_char - string] = '\0';
            if (max_chars_on_screen > 0)
            {
                size_screen = utf8_strlen_screen (utf_char);
                if (chars_displayed + size_screen > max_chars_on_screen)
                    return;
                chars_displayed += size_screen;
            }
            if (gui_window_utf_char_valid (utf_char))
            {
                output = string_iconv_from_internal (NULL, utf_char);
                wprintw (GUI_CURSES(window)->win_chat,
                         "%s", (output) ? output : utf_char);
                if (output)
                    free (output);
            }
            else
            {
                wprintw (GUI_CURSES(window)->win_chat, ".");
            }
        }
        
        string = next_char;
    }
}

/*
 * gui_chat_display_word: display a word on chat buffer
 */

void
gui_chat_display_word (struct t_gui_window *window,
                       struct t_gui_line *line,
                       char *data, char *end_offset,
                       int prefix, int num_lines, int count,
                       int *lines_displayed, int simulate)
{
    char *end_line, saved_char_end, saved_char, str_space[] = " ";
    int pos_saved_char, chars_to_display, num_displayed;
    int length_align;
    
    if (!data ||
        ((!simulate) && (window->win_chat_cursor_y >= window->win_chat_height)))
        return;
    
    end_line = data + strlen (data);
        
    if (end_offset && end_offset[0])
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
        length_align = gui_chat_get_line_align (window->buffer, line, 0);
        if ((length_align > 0) &&
            (window->win_chat_cursor_x == 0) &&
            (*lines_displayed > 0) &&
            /* TODO: modify arbitraty value for non aligning messages on time/nick? */
            (length_align < (window->win_chat_width - 5)))
        {
            if (!simulate)
            {
                wmove (GUI_CURSES(window)->win_chat,
                       window->win_chat_cursor_y,
                       window->win_chat_cursor_x);
                wclrtoeol (GUI_CURSES(window)->win_chat);
            }
            window->win_chat_cursor_x += length_align;
            if ((CONFIG_INTEGER(config_look_prefix_align) != CONFIG_LOOK_PREFIX_ALIGN_NONE)
                && (CONFIG_STRING(config_look_prefix_suffix)
                    && CONFIG_STRING(config_look_prefix_suffix)[0]))
            {
                if (!simulate)
                {
                    gui_chat_set_weechat_color (window, GUI_COLOR_CHAT_PREFIX_SUFFIX);
                    gui_chat_display_word_raw (window,
                                               CONFIG_STRING(config_look_prefix_suffix),
                                               0, 1);
                }
                window->win_chat_cursor_x += gui_chat_strlen_screen (CONFIG_STRING(config_look_prefix_suffix));
                if (!simulate)
                    gui_chat_display_word_raw (window, str_space, 0, 1);
                window->win_chat_cursor_x += gui_chat_strlen_screen (str_space);
                if (!simulate)
                    gui_chat_set_weechat_color (window, GUI_COLOR_CHAT);
            }
        }
        
        chars_to_display = gui_chat_strlen_screen (data);

        /* too long for current line */
        if (window->win_chat_cursor_x + chars_to_display > gui_chat_get_real_width (window))
        {
            num_displayed = gui_chat_get_real_width (window) - window->win_chat_cursor_x;
            pos_saved_char = gui_chat_string_real_pos (data, num_displayed);
            if (!simulate)
            {
                saved_char = data[pos_saved_char];
                data[pos_saved_char] = '\0';
                if ((count == 0) || (*lines_displayed >= num_lines - count))
                    gui_chat_display_word_raw (window, data, 0, 1);
                else
                    gui_chat_display_word_raw (window, data, 0, 0);
                data[pos_saved_char] = saved_char;
            }
            data += pos_saved_char;
        }
        else
        {
            num_displayed = chars_to_display;
            if (!simulate)
            {
                if ((count == 0) || (*lines_displayed >= num_lines - count))
                    gui_chat_display_word_raw (window, data, 0, 1);
                else
                    gui_chat_display_word_raw (window, data, 0, 0);
            }
            data += strlen (data);
        }
        
        window->win_chat_cursor_x += num_displayed;
        
        /* display new line? */
        if ((!prefix && (data >= end_line)) ||
            (((simulate) ||
              (window->win_chat_cursor_y <= window->win_chat_height - 1)) &&
             (window->win_chat_cursor_x > (gui_chat_get_real_width (window) - 1))))
            gui_chat_display_new_line (window, num_lines, count,
                                       lines_displayed, simulate);
        
        if ((!prefix && (data >= end_line)) ||
            ((!simulate) && (window->win_chat_cursor_y >= window->win_chat_height)))
            data = NULL;
    }
    
    if (end_offset)
        end_offset[1] = saved_char_end;
}

/*
 * gui_chat_display_time_and_prefix: display time and prefix for a line
 */

void
gui_chat_display_time_and_prefix (struct t_gui_window *window,
                                  struct t_gui_line *line,
                                  int num_lines, int count,
                                  int *lines_displayed,
                                  int simulate)
{
    char str_space[] = " ", str_plus[] = "+";
    int i, length_allowed, num_spaces;
    
    /* display time */
    if (line->str_time && line->str_time[0])
    {
        if (!simulate)
            gui_chat_reset_style (window);
        
        gui_chat_display_word (window, line, line->str_time,
                               NULL, 1, num_lines, count, lines_displayed,
                               simulate);
        gui_chat_display_word (window, line, str_space,
                               NULL, 1, num_lines, count, lines_displayed,
                               simulate);
    }
    
    /* display prefix */
    if (line->prefix
        && (line->prefix[0]
            || (CONFIG_INTEGER(config_look_prefix_align) != CONFIG_LOOK_PREFIX_ALIGN_NONE)))
    {
        if (!simulate)
            gui_chat_reset_style (window);
        
        if (CONFIG_INTEGER(config_look_prefix_align_max) > 0)
        {
            length_allowed =
                (window->buffer->prefix_max_length <= CONFIG_INTEGER(config_look_prefix_align_max)) ?
                window->buffer->prefix_max_length : CONFIG_INTEGER(config_look_prefix_align_max);
        }
        else
            length_allowed = window->buffer->prefix_max_length;
        
        num_spaces = length_allowed - line->prefix_length;
        
        if (CONFIG_INTEGER(config_look_prefix_align) == CONFIG_LOOK_PREFIX_ALIGN_RIGHT)
        {
            for (i = 0; i < num_spaces; i++)
            {
                gui_chat_display_word (window, line, str_space,
                                       NULL, 1, num_lines, count,
                                       lines_displayed,simulate);
            }
        }
        /* not enough space to display full prefix ? => truncate it! */
        if ((CONFIG_INTEGER(config_look_prefix_align) != CONFIG_LOOK_PREFIX_ALIGN_NONE)
            && (num_spaces < 0))
        {
            gui_chat_display_word (window, line, line->prefix,
                                   line->prefix +
                                   gui_chat_string_real_pos (line->prefix,
                                                             length_allowed - 1),
                                   1, num_lines, count, lines_displayed,
                                   simulate);
        }
        else
        {
            gui_chat_display_word (window, line, line->prefix,
                                   NULL, 1, num_lines, count, lines_displayed,
                                   simulate);
        }
        
        if (!simulate)
            gui_chat_reset_style (window);
        
        if (CONFIG_INTEGER(config_look_prefix_align) == CONFIG_LOOK_PREFIX_ALIGN_LEFT)
        {
            for (i = 0; i < num_spaces; i++)
            {
                gui_chat_display_word (window, line, str_space,
                                       NULL, 1, num_lines, count, lines_displayed,
                                       simulate);
            }
        }
        if ((CONFIG_INTEGER(config_look_prefix_align) != CONFIG_LOOK_PREFIX_ALIGN_NONE)
            && (num_spaces < 0))
        {
            if (!simulate)
                gui_chat_set_weechat_color (window, GUI_COLOR_CHAT_PREFIX_MORE);
            gui_chat_display_word (window, line, str_plus,
                                   NULL, 1, num_lines, count, lines_displayed,
                                   simulate);
        }
        else
        {
            gui_chat_display_word (window, line, str_space,
                                   NULL, 1, num_lines, count, lines_displayed,
                                   simulate);
        }
        if ((CONFIG_INTEGER(config_look_prefix_align) != CONFIG_LOOK_PREFIX_ALIGN_NONE)
            && (CONFIG_STRING(config_look_prefix_suffix)
                && CONFIG_STRING(config_look_prefix_suffix)[0]))
        {
            if (!simulate)
                gui_chat_set_weechat_color (window, GUI_COLOR_CHAT_PREFIX_SUFFIX);
            gui_chat_display_word (window, line,
                                   CONFIG_STRING(config_look_prefix_suffix),
                                   NULL, 1, num_lines, count,
                                   lines_displayed, simulate);
            gui_chat_display_word (window, line, str_space,
                                   NULL, 1, num_lines, count,
                                   lines_displayed, simulate);
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
gui_chat_display_line (struct t_gui_window *window, struct t_gui_line *line,
                       int count, int simulate)
{
    int num_lines, x, y, lines_displayed, line_align;
    int read_marker_x, read_marker_y;
    int word_start_offset, word_end_offset;
    int word_length_with_spaces, word_length;
    char *ptr_data, *ptr_end_offset, *next_char;
    char *ptr_style;
    
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
    if (line->str_time)
        read_marker_x = x + gui_chat_strlen_screen (line->str_time);
    else
        read_marker_x = x;
    read_marker_y = y;
    
    lines_displayed = 0;
    
    /* display time and prefix */
    gui_chat_display_time_and_prefix (window, line, num_lines, count,
                                      &lines_displayed, simulate);
    
    /* reset color & style for a new line */
    if (!simulate)
        gui_chat_reset_style (window);
    
    if (!line->message || !line->message[0])
        gui_chat_display_new_line (window, num_lines, count,
                                   &lines_displayed, simulate);
    else
    {
        ptr_data = line->message;
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
                line_align = gui_chat_get_line_align (window->buffer, line, 1);
                if ((window->win_chat_cursor_x + word_length_with_spaces > gui_chat_get_real_width (window))
                    && (word_length <= gui_chat_get_real_width (window) - line_align))
                {
                    gui_chat_display_new_line (window, num_lines, count,
                                               &lines_displayed, simulate);
                    /* apply styles before jumping to start of word */
                    if (!simulate && (word_start_offset > 0))
                    {
                        ptr_style = ptr_data;
                        while (ptr_style < ptr_data + word_start_offset)
                        {
                            /* loop until no style/char available */
                            ptr_style = gui_chat_string_next_char (window,
                                                                   (unsigned char *)ptr_style,
                                                                   1);
                            if (!ptr_style)
                                break;
                            ptr_style = utf8_next_char (ptr_style);
                        }
                    }
                    /* jump to start of word */
                    ptr_data += word_start_offset;
                }
                
                /* display word */
                gui_chat_display_word (window, line, ptr_data,
                                       ptr_end_offset,
                                       0, num_lines, count, &lines_displayed,
                                       simulate);
                
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
                            next_char = utf8_next_char (ptr_data);
                            if (!next_char)
                                break;
                            ptr_data = gui_chat_string_next_char (window,
                                                                  (unsigned char *)next_char,
                                                                  0);
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
    }
    
    if (simulate)
    {
        window->win_chat_cursor_x = x;
        window->win_chat_cursor_y = y;
    }
    else
    {
        if (CONFIG_STRING(config_look_read_marker)
            && CONFIG_STRING(config_look_read_marker)[0])
        {
            /* display marker if line is matching user search */
            if (window->buffer->text_search != GUI_TEXT_SEARCH_DISABLED)
            {
                if (gui_chat_line_search (line, window->buffer->input_buffer,
                                          window->buffer->text_search_exact))
                {
                    gui_chat_set_weechat_color (window,
                                                GUI_COLOR_CHAT_READ_MARKER);
                    mvwprintw (GUI_CURSES(window)->win_chat,
                               read_marker_y, read_marker_x,
                               "%c", CONFIG_STRING(config_look_read_marker)[0]);
                }
            }
            else
            {
                /* display read marker if needed */
                if (window->buffer->last_read_line &&
                    (window->buffer->last_read_line == gui_chat_get_prev_line_displayed (line)))
                {
                    gui_chat_set_weechat_color (window,
                                                GUI_COLOR_CHAT_READ_MARKER);
                    mvwprintw (GUI_CURSES(window)->win_chat,
                               read_marker_y, read_marker_x,
                               "%c", CONFIG_STRING(config_look_read_marker)[0]);
                }
            }
        }
    }
    
    return lines_displayed;
}

/*
 * gui_chat_display_line_y: display a line in the chat window (for a buffer
 *                          with free content)
 */

void
gui_chat_display_line_y (struct t_gui_window *window, struct t_gui_line *line,
                         int y)
{
    /* reset color & style for a new line */
    gui_chat_reset_style (window);
    
    window->win_chat_cursor_x = 0;
    window->win_chat_cursor_y = y;
    
    wmove (GUI_CURSES(window)->win_chat,
           window->win_chat_cursor_y,
           window->win_chat_cursor_x);
    wclrtoeol (GUI_CURSES(window)->win_chat);
    
    gui_chat_display_word_raw (window, line->message,
                               window->win_chat_width, 1);
}

/*
 * gui_chat_calculate_line_diff: returns pointer to line & offset for a
 *                               difference with given line
 */

void
gui_chat_calculate_line_diff (struct t_gui_window *window,
                              struct t_gui_line **line, int *line_pos,
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
            *line = gui_chat_get_last_line_displayed (window->buffer);
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
            *line = gui_chat_get_first_line_displayed (window->buffer);
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
                *line = gui_chat_get_prev_line_displayed (*line);
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
                *line = gui_chat_get_next_line_displayed (*line);
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
            *line = gui_chat_get_first_line_displayed (window->buffer);
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
    struct t_gui_window *ptr_win;
    struct t_gui_line *ptr_line;
    /*t_irc_dcc *dcc_first, *dcc_selected, *ptr_dcc;*/
    char format_empty[32];
    int i, line_pos, count, old_scroll, y_start, y_end;
    /*int j, num_bars;
    unsigned long pct_complete;
    char *unit_name[] = { N_("bytes"), N_("KB"), N_("MB"), N_("GB") };
    char *unit_format[] = { "%.0f", "%.1f", "%.02f", "%.02f" };
    float unit_divide[] = { 1, 1024, 1024*1024, 1024*1024*1024 };
    int num_unit;
    char format[32], date[128], *buf;
    struct tm *date_tmp;*/
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            if (erase)
            {
                gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_chat,
                                              GUI_COLOR_CHAT);
                
                snprintf (format_empty, 32, "%%-%ds", ptr_win->win_chat_width);
                for (i = 0; i < ptr_win->win_chat_height; i++)
                {
                    mvwprintw (GUI_CURSES(ptr_win)->win_chat, i, 0,
                               format_empty, " ");
                }
            }
            
            gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_chat,
                                          GUI_COLOR_CHAT);
            
            /*if (buffer->type == GUI_BUFFER_TYPE_DCC)
            {
                i = 0;
                dcc_first = (ptr_win->dcc_first) ? (t_irc_dcc *) ptr_win->dcc_first : irc_dcc_list;
                dcc_selected = (ptr_win->dcc_selected) ? (t_irc_dcc *) ptr_win->dcc_selected : irc_dcc_list;
                for (ptr_dcc = dcc_first; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
                {
                    if (i >= ptr_win->win_chat_height - 1)
                        break;
                    
                    // nickname and filename
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_chat,
                                                  (ptr_dcc == dcc_selected) ?
                                                  GUI_COLOR_DCC_SELECTED : GUI_COLOR_WIN_CHAT);
                    mvwprintw (GUI_CURSES(ptr_win)->win_chat, i, 0, "%s %-16s ",
                               (ptr_dcc == dcc_selected) ? "***" : "   ",
                               ptr_dcc->nick);
                    buf = weechat_iconv_from_internal (NULL,
                                                       (IRC_DCC_IS_CHAT(ptr_dcc->type)) ?
                                                       _(ptr_dcc->filename) : ptr_dcc->filename);
                    wprintw (GUI_CURSES(ptr_win)->win_chat, "%s",
                             (buf) ? buf : ((IRC_DCC_IS_CHAT(ptr_dcc->type)) ?
                             _(ptr_dcc->filename) : ptr_dcc->filename));
                    if (buf)
                        free (buf);
                    if (IRC_DCC_IS_FILE(ptr_dcc->type))
                    {
                        if (ptr_dcc->filename_suffix > 0)
                            wprintw (GUI_CURSES(ptr_win)->win_chat, " (.%d)",
                                     ptr_dcc->filename_suffix);
                    }
                    
                    // status
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_chat,
                                                  (ptr_dcc == dcc_selected) ?
                                                  GUI_COLOR_DCC_SELECTED : GUI_COLOR_WIN_CHAT);
                    mvwprintw (GUI_CURSES(ptr_win)->win_chat, i + 1, 0, "%s %s ",
                               (ptr_dcc == dcc_selected) ? "***" : "   ",
                               (IRC_DCC_IS_RECV(ptr_dcc->type)) ? "-->>" : "<<--");
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_chat,
                                                  GUI_COLOR_DCC_WAITING + ptr_dcc->status);
                    buf = weechat_iconv_from_internal (NULL, _(irc_dcc_status_string[ptr_dcc->status]));
                    wprintw (GUI_CURSES(ptr_win)->win_chat, "%-10s",
                             (buf) ? buf : _(irc_dcc_status_string[ptr_dcc->status]));
                    if (buf)
                        free (buf);
                    
                    // other infos
                    gui_window_set_weechat_color (GUI_CURSES(ptr_win)->win_chat,
                                                  (ptr_dcc == dcc_selected) ?
                                                  GUI_COLOR_DCC_SELECTED : GUI_COLOR_WIN_CHAT);
                    if (IRC_DCC_IS_FILE(ptr_dcc->type))
                    {
                        wprintw (GUI_CURSES(ptr_win)->win_chat, "  [");
                        if (ptr_dcc->size == 0)
                        {
                            if (ptr_dcc->status == IRC_DCC_DONE)
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
                            if (ptr_dcc->status == IRC_DCC_DONE)
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
                        if (ptr_dcc->status == IRC_DCC_ACTIVE)
                        {
                            wprintw (GUI_CURSES(ptr_win)->win_chat, _("ETA"));
                            wprintw (GUI_CURSES(ptr_win)->win_chat, ": %.2lu:%.2lu:%.2lu - ",
                                     ptr_dcc->eta / 3600,
                                     (ptr_dcc->eta / 60) % 60,
                                     ptr_dcc->eta % 60);
                        }
                        sprintf (format, "%s %%s/s)", unit_format[num_unit]);
                        buf = weechat_iconv_from_internal (NULL, unit_name[num_unit]);
                        wprintw (GUI_CURSES(ptr_win)->win_chat, format,
                                 ((float)ptr_dcc->bytes_per_sec) / ((float)(unit_divide[num_unit])),
                                 (buf) ? buf : unit_name[num_unit]);
                        if (buf)
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
            else*/
            {
                ptr_win->win_chat_cursor_x = 0;
                ptr_win->win_chat_cursor_y = 0;

                switch (ptr_win->buffer->type)
                {
                    case GUI_BUFFER_TYPE_FORMATED:
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
                            ptr_line = gui_chat_get_next_line_displayed (ptr_line);
                            ptr_win->first_line_displayed = 0;
                        }
                        else
                            ptr_win->first_line_displayed =
                                (ptr_line == gui_chat_get_first_line_displayed (ptr_win->buffer));
                        
                        /* display lines */
                        count = 0;
                        while (ptr_line && (ptr_win->win_chat_cursor_y <= ptr_win->win_chat_height - 1))
                        {
                            count = gui_chat_display_line (ptr_win, ptr_line, 0, 0);
                            ptr_line = gui_chat_get_next_line_displayed (ptr_line);
                        }
                        
                        old_scroll = ptr_win->scroll;
                        
                        ptr_win->scroll = (ptr_win->win_chat_cursor_y > ptr_win->win_chat_height - 1);
                        
                        /* check if last line of buffer is entirely displayed and scrolling */
                        /* if so, disable scroll indicator */
                        if (!ptr_line && ptr_win->scroll)
                        {
                            if (count == gui_chat_display_line (ptr_win, ptr_win->buffer->last_line, 0, 1))
                                ptr_win->scroll = 0;
                        }
                        
                        if (ptr_win->scroll != old_scroll)
                        {
                            hook_signal_send ("window_scrolled",
                                              WEECHAT_HOOK_SIGNAL_POINTER, ptr_win);
                        }
                        
                        if (!ptr_win->scroll
                            && (ptr_win->start_line == gui_chat_get_first_line_displayed (ptr_win->buffer)))
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
                        break;
                    case GUI_BUFFER_TYPE_FREE:
                        /* display at position of scrolling */
                        ptr_line = (ptr_win->start_line) ?
                            ptr_win->start_line : buffer->lines;
                        if (ptr_line)
                        {
                            if (!ptr_line->displayed)
                                ptr_line = gui_chat_get_next_line_displayed (ptr_line);
                            if (ptr_line)
                            {
                                y_start = (ptr_win->start_line) ? ptr_line->y : 0;
                                y_end = y_start + ptr_win->win_chat_height - 1;
                                while (ptr_line && (ptr_line->y <= y_end))
                                {
                                    if (ptr_line->refresh_needed || erase)
                                    {
                                        gui_chat_display_line_y (ptr_win, ptr_line,
                                                                 ptr_line->y - y_start);
                                    }
                                    ptr_line = gui_chat_get_next_line_displayed (ptr_line);
                                }
                            }
                        }
                }
            }
            wnoutrefresh (GUI_CURSES(ptr_win)->win_chat);
            refresh ();
            
            if (buffer->type == GUI_BUFFER_TYPE_FREE)
            {
                for (ptr_line = buffer->lines; ptr_line;
                     ptr_line = ptr_line->next_line)
                {
                    ptr_line->refresh_needed = 0;
                }
            }
        }
    }
}

/*
 * gui_chat_draw_line: add a line to chat window for a buffer
 */

void
gui_chat_draw_line (struct t_gui_buffer *buffer, struct t_gui_line *line)
{
    /* This function does nothing in Curses GUI,
       line will be displayed by gui_buffer_draw_chat()  */
    (void) buffer;
    (void) line;
}
