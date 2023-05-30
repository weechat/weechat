/*
 * gui-curses-chat.c - chat display functions for Curses GUI
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-eval.h"
#include "../../core/wee-hashtable.h"
#include "../../core/wee-hook.h"
#include "../../core/wee-string.h"
#include "../../core/wee-utf8.h"
#include "../../plugins/plugin.h"
#include "../gui-buffer.h"
#include "../gui-chat.h"
#include "../gui-color.h"
#include "../gui-hotlist.h"
#include "../gui-line.h"
#include "../gui-main.h"
#include "../gui-window.h"
#include "gui-curses.h"
#include "gui-curses-main.h"
#include "gui-curses-window.h"


/*
 * Gets real width for chat.
 *
 * Returns width - 1 if nicklist is at right, for good copy/paste (without
 * nicklist separator).
 */

int
gui_chat_get_real_width (struct t_gui_window *window)
{
    if (CONFIG_BOOLEAN(config_look_chat_space_right)
        && (window->win_chat_width > 1)
        && (window->win_chat_x + window->win_chat_width < gui_window_get_width ()))
    {
        return window->win_chat_width - 1;
    }
    else
    {
        return window->win_chat_width;
    }
}

/*
 * Checks if marker must be displayed after this line.
 *
 * Returns:
 *   1: marker must be displayed after this line
 *   0: marker must not be displayed after this line
 */

int
gui_chat_marker_for_line (struct t_gui_buffer *buffer, struct t_gui_line *line)
{
    struct t_gui_line *last_read_line;

    /* marker is disabled in config? */
    if (CONFIG_INTEGER(config_look_read_marker) != CONFIG_LOOK_READ_MARKER_LINE)
        return 0;

    /* marker is not set for buffer? */
    if (!buffer->lines->last_read_line)
        return 0;

    last_read_line = buffer->lines->last_read_line;
    if (!last_read_line->data->displayed)
        last_read_line = gui_line_get_prev_displayed (last_read_line);

    if (!last_read_line)
        return 0;

    while (line)
    {
        if (last_read_line == line)
        {
            if (CONFIG_BOOLEAN(config_look_read_marker_always_show))
                return 1;
            return (gui_line_get_next_displayed (line) != NULL) ? 1 : 0;
        }

        if (line->data->displayed)
            break;

        line = line->next_line;
    }
    return 0;
}

/*
 * Resets style using color depending on window (active or not) and line (buffer
 * selected in merged buffer or not).
 */

void
gui_chat_reset_style (struct t_gui_window *window, struct t_gui_line *line,
                      int nick_offline, int reset_attributes,
                      int color_inactive_window, int color_inactive_buffer,
                      int color_default)
{
    int color;

    color = color_default;
    if ((window != gui_current_window)
        && CONFIG_BOOLEAN(config_look_color_inactive_window))
    {
        color = color_inactive_window;
    }
    else if (line && !line->data->buffer->active
             && CONFIG_BOOLEAN(config_look_color_inactive_buffer))
    {
        color = color_inactive_buffer;
    }
    else if (nick_offline == 1)
    {
        color = GUI_COLOR_CHAT_NICK_OFFLINE;
    }
    else if (nick_offline > 1)
    {
        color = GUI_COLOR_CHAT_NICK_OFFLINE_HIGHLIGHT;
    }

    if (reset_attributes)
        gui_window_reset_style (GUI_WINDOW_OBJECTS(window)->win_chat, color);
    else
        gui_window_reset_color (GUI_WINDOW_OBJECTS(window)->win_chat, color);
}

/*
 * Deletes all chars from the cursor to the end of the current line.
 */

void
gui_chat_clrtoeol (struct t_gui_window *window)
{
    if (window->win_chat_cursor_y >= window->win_chat_height)
        return;

    wmove (GUI_WINDOW_OBJECTS(window)->win_chat,
           window->win_chat_cursor_y,
           window->win_chat_cursor_x);
    wclrtoeol (GUI_WINDOW_OBJECTS(window)->win_chat);
}

/*
 * Displays a new line.
 */

void
gui_chat_display_new_line (struct t_gui_window *window,
                           int num_lines, int count,
                           int *lines_displayed, int simulate)
{
    if ((count == 0) || (*lines_displayed >= num_lines - count))
        window->win_chat_cursor_y++;

    window->win_chat_cursor_x = 0;
    (*lines_displayed)++;

    if (!simulate)
        gui_chat_clrtoeol (window);
}

/*
 * Displays an horizontal line (marker for data not read).
 */

void
gui_chat_display_horizontal_line (struct t_gui_window *window, int simulate)
{
    int x, size_on_screen;
    char *read_marker_string, *default_string = "- ";

    if (simulate
        || (!simulate && (window->win_chat_cursor_y >= window->win_chat_height)))
        return;

    gui_window_coords_init_line (window, window->win_chat_cursor_y);
    if (CONFIG_INTEGER(config_look_read_marker) == CONFIG_LOOK_READ_MARKER_LINE)
    {
        read_marker_string = CONFIG_STRING(config_look_read_marker_string);
        if (!read_marker_string || !read_marker_string[0])
            read_marker_string = default_string;
        size_on_screen = utf8_strlen_screen (read_marker_string);
        gui_window_set_weechat_color (GUI_WINDOW_OBJECTS(window)->win_chat,
                                      GUI_COLOR_CHAT_READ_MARKER);
        gui_chat_clrtoeol (window);
        x = 0;
        while (x < gui_chat_get_real_width (window))
        {
            mvwaddstr (GUI_WINDOW_OBJECTS(window)->win_chat,
                       window->win_chat_cursor_y, x,
                       read_marker_string);
            x += size_on_screen;
        }
    }
    window->win_chat_cursor_x = window->win_chat_width;
}

/*
 * Returns next char of a word (for display), special chars like
 * colors/attributes are skipped and optionally applied.
 */

const char *
gui_chat_string_next_char (struct t_gui_window *window, struct t_gui_line *line,
                           const unsigned char *string, int apply_style,
                           int apply_style_inactive,
                           int nick_offline)
{
    if (apply_style && apply_style_inactive)
    {
        if ((window != gui_current_window)
            && CONFIG_BOOLEAN(config_look_color_inactive_window))
        {
            apply_style = 0;
        }
        else if (line && !line->data->buffer->active)
        {
            if (CONFIG_BOOLEAN(config_look_color_inactive_buffer))
                apply_style = 0;
        }
    }
    if (apply_style && nick_offline)
    {
        apply_style = 0;
    }

    while (string[0])
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
                                                          (apply_style) ? GUI_WINDOW_OBJECTS(window)->win_chat : NULL);
                        break;
                    case GUI_COLOR_BG_CHAR: /* bg color */
                        string++;
                        gui_window_string_apply_color_bg ((unsigned char **)&string,
                                                          (apply_style) ? GUI_WINDOW_OBJECTS(window)->win_chat : NULL);
                        break;
                    case GUI_COLOR_FG_BG_CHAR: /* fg + bg color */
                        string++;
                        gui_window_string_apply_color_fg_bg ((unsigned char **)&string,
                                                             (apply_style) ? GUI_WINDOW_OBJECTS(window)->win_chat : NULL);
                        break;
                    case GUI_COLOR_EXTENDED_CHAR: /* pair number */
                        string++;
                        gui_window_string_apply_color_pair ((unsigned char **)&string,
                                                            (apply_style) ? GUI_WINDOW_OBJECTS(window)->win_chat : NULL);
                        break;
                    case GUI_COLOR_EMPHASIS_CHAR: /* emphasis */
                        string++;
                        if (apply_style)
                            gui_window_toggle_emphasis ();
                        break;
                    case GUI_COLOR_BAR_CHAR: /* bar color */
                        string++;
                        switch (string[0])
                        {
                            case GUI_COLOR_BAR_FG_CHAR:
                            case GUI_COLOR_BAR_DELIM_CHAR:
                            case GUI_COLOR_BAR_BG_CHAR:
                            case GUI_COLOR_BAR_START_INPUT_CHAR:
                            case GUI_COLOR_BAR_START_INPUT_HIDDEN_CHAR:
                            case GUI_COLOR_BAR_MOVE_CURSOR_CHAR:
                            case GUI_COLOR_BAR_START_ITEM:
                            case GUI_COLOR_BAR_START_LINE_ITEM:
                            case GUI_COLOR_BAR_SPACER:
                                string++;
                                break;
                        }
                        break;
                    case GUI_COLOR_RESET_CHAR: /* reset color (keep attributes) */
                        string++;
                        if (apply_style)
                        {
                            if (apply_style_inactive)
                            {
                                gui_chat_reset_style (window, line, 0, 0,
                                                      GUI_COLOR_CHAT_INACTIVE_WINDOW,
                                                      GUI_COLOR_CHAT_INACTIVE_BUFFER,
                                                      GUI_COLOR_CHAT);
                            }
                            else
                            {
                                gui_window_reset_color (GUI_WINDOW_OBJECTS(window)->win_chat,
                                                        GUI_COLOR_CHAT);
                            }
                        }
                        break;
                    default:
                        gui_window_string_apply_color_weechat ((unsigned char **)&string,
                                                               (apply_style) ? GUI_WINDOW_OBJECTS(window)->win_chat : NULL);
                        break;
                }
                break;
            case GUI_COLOR_SET_ATTR_CHAR:
                string++;
                gui_window_string_apply_color_set_attr ((unsigned char **)&string,
                                                        (apply_style) ? GUI_WINDOW_OBJECTS(window)->win_chat : NULL);
                break;
            case GUI_COLOR_REMOVE_ATTR_CHAR:
                string++;
                gui_window_string_apply_color_remove_attr ((unsigned char **)&string,
                                                           (apply_style) ? GUI_WINDOW_OBJECTS(window)->win_chat : NULL);
                break;
            case GUI_COLOR_RESET_CHAR:
                string++;
                if (apply_style)
                {
                    if (apply_style_inactive)
                    {
                        gui_chat_reset_style (window, line, 0, 1,
                                              GUI_COLOR_CHAT_INACTIVE_WINDOW,
                                              GUI_COLOR_CHAT_INACTIVE_BUFFER,
                                              GUI_COLOR_CHAT);
                    }
                    else
                    {
                        gui_window_reset_style (GUI_WINDOW_OBJECTS(window)->win_chat,
                                                GUI_COLOR_CHAT);
                    }
                }
                break;
            default:
                return (char *)string;
                break;
        }
    }

    /* nothing found except color/attrib codes, so return NULL */
    return NULL;
}

/*
 * Displays word on chat buffer, letter by letter, special chars like
 * colors/attributes are interpreted.
 *
 * Returns number of chars displayed on screen.
 */

int
gui_chat_display_word_raw (struct t_gui_window *window, struct t_gui_line *line,
                           const char *string,
                           int max_chars_on_screen, int simulate,
                           int apply_style_inactive,
                           int nick_offline)
{
    const char *ptr_char;
    char *output, utf_char[16], utf_char2[16];
    int x, chars_displayed, display_char, size_on_screen, reverse_video;

    if (!simulate)
    {
        wmove (GUI_WINDOW_OBJECTS(window)->win_chat,
               window->win_chat_cursor_y,
               window->win_chat_cursor_x);
    }

    chars_displayed = 0;
    x = window->win_chat_cursor_x;

    while (string && string[0])
    {
        string = gui_chat_string_next_char (window, line,
                                            (unsigned char *)string, 1,
                                            apply_style_inactive,
                                            nick_offline);
        if (!string)
            return chars_displayed;

        utf8_strncpy (utf_char, string, 1);
        if (utf_char[0])
        {
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

            display_char = (window->buffer->type != GUI_BUFFER_TYPE_FREE)
                || (x >= window->scroll->start_col);

            size_on_screen = utf8_strlen_screen (ptr_char);
            if ((max_chars_on_screen > 0)
                && (chars_displayed + size_on_screen > max_chars_on_screen))
            {
                return chars_displayed;
            }

            if (display_char)
            {
                while (ptr_char && ptr_char[0])
                {
                    utf8_strncpy (utf_char2, ptr_char, 1);
                    size_on_screen = utf8_char_size_screen (utf_char2);
                    if (size_on_screen >= 0)
                    {
                        if (!simulate)
                        {
                            output = string_iconv_from_internal (NULL, utf_char2);
                            if (reverse_video)
                            {
                                wattron (GUI_WINDOW_OBJECTS(window)->win_chat,
                                         A_REVERSE);
                            }
                            waddstr (GUI_WINDOW_OBJECTS(window)->win_chat,
                                     (output) ? output : utf_char2);
                            if (reverse_video)
                            {
                                wattroff (GUI_WINDOW_OBJECTS(window)->win_chat,
                                          A_REVERSE);
                            }
                            if (output)
                                free (output);

                            if (gui_window_current_emphasis)
                            {
                                gui_window_emphasize (GUI_WINDOW_OBJECTS(window)->win_chat,
                                                      x - window->scroll->start_col,
                                                      window->win_chat_cursor_y,
                                                      size_on_screen);
                            }
                        }
                        chars_displayed += size_on_screen;
                        x += size_on_screen;
                    }
                    ptr_char = utf8_next_char (ptr_char);
                }
            }
            else
            {
                x += size_on_screen;
            }
        }

        string = utf8_next_char (string);
    }

    return chars_displayed;
}

/*
 * Displays the prefix_suffix in the beginning of a line.
 *
 * Returns number of chars displayed on screen.
 */

int
gui_chat_display_prefix_suffix (struct t_gui_window *window,
                                struct t_gui_line *line,
                                const char *word,
                                int pre_lines_displayed, int *lines_displayed,
                                int pre_chars_displayed,
                                int simulate,
                                int apply_style_inactive,
                                int nick_offline)
{
    char str_space[] = " ";
    int chars_displayed, length_align;

    chars_displayed = 0;

    if ((!simulate) && (window->win_chat_cursor_y >= window->win_chat_height))
        return chars_displayed;

    /* insert spaces for aligning text under time/nick */
    length_align = gui_line_get_align (window->buffer, line, 0, 0);

    /* in the beginning of a line */
    if ((window->win_chat_cursor_x == 0)
        && (*lines_displayed > pre_lines_displayed)
        /* FIXME: modify arbitrary value for non aligning messages on time/nick? */
        && (length_align < (window->win_chat_width - 5)))
    {
        /* in the beginning of a word or in the middle of a word with multiline word align */
        if ((pre_chars_displayed + chars_displayed == 0)
            || CONFIG_BOOLEAN(config_look_align_multiline_words))
        {
            window->win_chat_cursor_x += length_align;

            if ((CONFIG_INTEGER(config_look_align_end_of_lines) == CONFIG_LOOK_ALIGN_END_OF_LINES_MESSAGE)
                && (CONFIG_INTEGER(config_look_prefix_align) != CONFIG_LOOK_PREFIX_ALIGN_NONE)
                && CONFIG_STRING(config_look_prefix_suffix)
                && CONFIG_STRING(config_look_prefix_suffix)[0]
                && line->data->date > 0)
            {
                if (!simulate)
                {
                    gui_window_save_style (GUI_WINDOW_OBJECTS(window)->win_chat);
                    gui_window_set_weechat_color (GUI_WINDOW_OBJECTS(window)->win_chat,
                                                  GUI_COLOR_CHAT_PREFIX_SUFFIX);
                    gui_window_current_emphasis = 0;
                }
                chars_displayed += gui_chat_display_word_raw (window, line,
                                                              CONFIG_STRING(config_look_prefix_suffix),
                                                              0, simulate,
                                                              apply_style_inactive,
                                                              nick_offline);
                window->win_chat_cursor_x += gui_chat_strlen_screen (CONFIG_STRING(config_look_prefix_suffix));
                chars_displayed += gui_chat_display_word_raw (window, line,
                                                              str_space,
                                                              0, simulate,
                                                              apply_style_inactive,
                                                              nick_offline);
                window->win_chat_cursor_x += gui_chat_strlen_screen (str_space);
                if (!simulate)
                    gui_window_restore_style (GUI_WINDOW_OBJECTS(window)->win_chat);
            }
        }
        if (!simulate && (window->win_chat_cursor_y < window->coords_size))
        {
            window->coords[window->win_chat_cursor_y].line = line;
            window->coords[window->win_chat_cursor_y].data = (char *)word;
        }
    }

    return chars_displayed;
}

/*
 * Displays a word on chat buffer.
 *
 * Returns number of chars displayed on screen.
 */

int
gui_chat_display_word (struct t_gui_window *window,
                       struct t_gui_line *line,
                       const char *word, const char *word_end,
                       int prefix, int num_lines, int count,
                       int pre_lines_displayed, int *lines_displayed,
                       int simulate,
                       int apply_style_inactive,
                       int nick_offline)
{
    char *data, *ptr_data, *end_line, saved_char;
    int chars_displayed, pos_saved_char, chars_to_display, num_displayed;

    chars_displayed = 0;

    if (!word ||
        ((!simulate) && (window->win_chat_cursor_y >= window->win_chat_height)))
        return chars_displayed;

    if (!simulate && (window->win_chat_cursor_y < window->coords_size))
        window->coords[window->win_chat_cursor_y].line = line;

    data = strdup (word);
    if (!data)
        return chars_displayed;

    end_line = data + strlen (data);

    if (word_end && word_end[0])
        data[word_end - word] = '\0';
    else
        word_end = NULL;

    ptr_data = data;
    while (ptr_data && ptr_data[0])
    {
        chars_displayed += gui_chat_display_prefix_suffix(window, line,
                                                          word + (ptr_data - data),
                                                          pre_lines_displayed,
                                                          lines_displayed,
                                                          chars_displayed,
                                                          simulate,
                                                          apply_style_inactive,
                                                          nick_offline);

        chars_to_display = gui_chat_strlen_screen (ptr_data);

        /* too long for current line */
        if (window->win_chat_cursor_x + chars_to_display > gui_chat_get_real_width (window))
        {
            num_displayed = gui_chat_get_real_width (window) - window->win_chat_cursor_x;
            pos_saved_char = gui_chat_string_real_pos (ptr_data, num_displayed,
                                                       1);
            saved_char = ptr_data[pos_saved_char];
            ptr_data[pos_saved_char] = '\0';
            if ((count == 0) || (*lines_displayed >= num_lines - count))
            {
                chars_displayed += gui_chat_display_word_raw (window, line,
                                                              ptr_data,
                                                              0, simulate,
                                                              apply_style_inactive,
                                                              nick_offline);
            }
            else
            {
                chars_displayed += gui_chat_display_word_raw (window, line,
                                                              ptr_data,
                                                              0, 0,
                                                              apply_style_inactive,
                                                              nick_offline);
            }
            ptr_data[pos_saved_char] = saved_char;
            ptr_data += pos_saved_char;
        }
        else
        {
            num_displayed = chars_to_display;
            if ((count == 0) || (*lines_displayed >= num_lines - count))
            {
                chars_displayed += gui_chat_display_word_raw (window, line,
                                                              ptr_data,
                                                              0, simulate,
                                                              apply_style_inactive,
                                                              nick_offline);
            }
            else
            {
                chars_displayed += gui_chat_display_word_raw (window, line,
                                                              ptr_data,
                                                              0, 0,
                                                              apply_style_inactive,
                                                              nick_offline);
            }
            ptr_data += strlen (ptr_data);
        }

        window->win_chat_cursor_x += num_displayed;

        /* display new line? */
        if ((!prefix && (ptr_data >= end_line)) ||
            (((simulate) ||
              (window->win_chat_cursor_y <= window->win_chat_height - 1)) &&
             (window->win_chat_cursor_x > (gui_chat_get_real_width (window) - 1))))
            gui_chat_display_new_line (window, num_lines, count,
                                       lines_displayed, simulate);

        if ((!prefix && (ptr_data >= end_line)) ||
            ((!simulate) && (window->win_chat_cursor_y >= window->win_chat_height)))
            ptr_data = NULL;
    }

    free (data);

    return chars_displayed;
}

/*
 * Displays a message when the day has changed.
 */

void
gui_chat_display_day_changed (struct t_gui_window *window,
                              struct tm *date1, struct tm *date2,
                              int simulate)
{
    char temp_message[1024], message[1024], *message_with_color;
    int year1, year1_last_yday;

    if (simulate
        || (!simulate && (window->win_chat_cursor_y >= window->win_chat_height)))
        return;

    /*
     * if date1 is given, compare date1 and date2; if date2 is date1 + 1 day,
     * do not display date1 (so wee keep date1 if date2 is > date1 + 1 day)
     */
    if (date1)
    {
        if (date1->tm_year == date2->tm_year)
        {
            if (date1->tm_yday == date2->tm_yday - 1)
                date1 = NULL;
        }
        else if ((date1->tm_year == date2->tm_year - 1) && (date2->tm_yday == 0))
        {
            /* date2 is 01/01, then check if date1 is 31/12 */
            year1 = date1->tm_year + 1900;
            year1_last_yday = (((year1 % 400) == 0)
                              || (((year1 % 4) == 0) && ((year1 % 100) != 0))) ?
                365 : 364;
            if (date1->tm_yday == year1_last_yday)
                date1 = NULL;
        }
    }

    /* build the message to display */
    if (date1)
    {
        if (strftime (temp_message, sizeof (temp_message),
                      CONFIG_STRING(config_look_day_change_message_2dates),
                      date1) == 0)
            temp_message[0] = '\0';
        if (strftime (message, sizeof (message), temp_message, date2) == 0)
            message[0] = '\0';
    }
    else
    {
        if (strftime (message, sizeof (message),
                      CONFIG_STRING(config_look_day_change_message_1date),
                      date2) == 0)
            message[0] = '\0';
    }

    message_with_color = (strstr (message, "${")) ?
        eval_expression (message, NULL, NULL, NULL) : NULL;

    /* display the message */
    gui_window_coords_init_line (window, window->win_chat_cursor_y);
    gui_window_set_weechat_color (GUI_WINDOW_OBJECTS(window)->win_chat,
                                  GUI_COLOR_CHAT_DAY_CHANGE);
    gui_chat_clrtoeol (window);
    gui_chat_display_word_raw (window, NULL,
                               (message_with_color) ? message_with_color : message,
                               0, simulate, 0, 0);
    window->win_chat_cursor_x = window->win_chat_width;

    if (message_with_color)
        free (message_with_color);
}

/*
 * Checks if time on line is the same as time on previous line.
 *
 * Returns:
 *   1: time is same as time on previous line
 *   0: time is different from time on previous line
 */

int
gui_chat_line_time_is_same_as_previous (struct t_gui_line *line)
{
    struct t_gui_line *prev_line;

    /* previous line is not found => display standard time */
    prev_line = gui_line_get_prev_displayed (line);
    if (!prev_line)
        return 0;

    /* previous line has no time => display standard time */
    if (!line->data->str_time || !prev_line->data->str_time)
        return 0;

    /* time can be hidden/replaced if times are equal */
    return (strcmp (line->data->str_time, prev_line->data->str_time) == 0) ?
        1 : 0;
}

/*
 * Displays time, buffer name (for merged buffers) and prefix for a line.
 */

void
gui_chat_display_time_to_prefix (struct t_gui_window *window,
                                 struct t_gui_line *line,
                                 int num_lines, int count,
                                 int pre_lines_displayed,
                                 int *lines_displayed,
                                 int simulate)
{
    char str_space[] = " ";
    char *prefix_no_color, *prefix_highlighted, *ptr_prefix, *ptr_prefix2;
    char *ptr_prefix_color;
    const char *short_name, *str_color, *ptr_nick_prefix, *ptr_nick_suffix;
    int i, length, length_allowed, num_spaces, prefix_length, extra_spaces;
    int chars_displayed, nick_offline, prefix_is_nick, length_nick_prefix_suffix;
    int chars_to_display;
    struct t_gui_lines *mixed_lines;

    if (!simulate)
    {
        if (window->win_chat_cursor_y < window->coords_size)
            window->coords[window->win_chat_cursor_y].line = line;
        gui_chat_reset_style (window, line, 0, 1,
                              GUI_COLOR_CHAT_INACTIVE_WINDOW,
                              GUI_COLOR_CHAT_INACTIVE_BUFFER,
                              GUI_COLOR_CHAT);
        gui_chat_clrtoeol (window);
    }

    /* display time */
    if (window->buffer->time_for_each_line
        && line->data->str_time && line->data->str_time[0])
    {
        if (window->win_chat_cursor_y < window->coords_size)
            window->coords[window->win_chat_cursor_y].time_x1 = window->win_chat_cursor_x;

        if (CONFIG_STRING(config_look_buffer_time_same)
            && CONFIG_STRING(config_look_buffer_time_same)[0]
            && gui_chat_line_time_is_same_as_previous (line))
        {
            length_allowed = gui_chat_time_length;
            num_spaces = length_allowed - gui_chat_strlen_screen (
                config_buffer_time_same_evaluated);
            gui_chat_display_word (
                window, line, config_buffer_time_same_evaluated,
                (num_spaces < 0) ?
                config_buffer_time_same_evaluated +
                gui_chat_string_real_pos (config_buffer_time_same_evaluated,
                                          length_allowed,
                                          1) :
                NULL,
                1, num_lines, count,
                pre_lines_displayed, lines_displayed,
                simulate,
                CONFIG_BOOLEAN(config_look_color_inactive_time),
                0);
            for (i = 0; i < num_spaces; i++)
            {
                gui_chat_display_word (
                    window, line, str_space,
                    NULL, 1, num_lines, count,
                    pre_lines_displayed, lines_displayed,
                    simulate,
                    CONFIG_BOOLEAN(config_look_color_inactive_time),
                    0);
            }
        }
        else
        {
            gui_chat_display_word (
                window, line, line->data->str_time,
                NULL, 1, num_lines, count,
                pre_lines_displayed, lines_displayed,
                simulate,
                CONFIG_BOOLEAN(config_look_color_inactive_time),
                0);
        }

        if (window->win_chat_cursor_y < window->coords_size)
            window->coords[window->win_chat_cursor_y].time_x2 = window->win_chat_cursor_x - 1;

        if (!simulate)
        {
            gui_chat_reset_style (window, line, 0, 1,
                                  GUI_COLOR_CHAT_INACTIVE_WINDOW,
                                  GUI_COLOR_CHAT_INACTIVE_BUFFER,
                                  GUI_COLOR_CHAT);
        }
        gui_chat_display_word (window, line, str_space,
                               NULL, 1, num_lines, count,
                               pre_lines_displayed, lines_displayed,
                               simulate,
                               CONFIG_BOOLEAN(config_look_color_inactive_time),
                               0);
    }

    /* display buffer name (if many buffers are merged) */
    mixed_lines = line->data->buffer->mixed_lines;
    if (mixed_lines && (line->data->buffer->active != 2))
    {
        if ((CONFIG_INTEGER(config_look_prefix_buffer_align_max) > 0)
            && (CONFIG_INTEGER(config_look_prefix_buffer_align) != CONFIG_LOOK_PREFIX_BUFFER_ALIGN_NONE))
        {
            length_allowed =
                (mixed_lines->buffer_max_length <= CONFIG_INTEGER(config_look_prefix_buffer_align_max)) ?
                mixed_lines->buffer_max_length : CONFIG_INTEGER(config_look_prefix_buffer_align_max);
        }
        else
            length_allowed = mixed_lines->buffer_max_length;

        short_name = gui_buffer_get_short_name (line->data->buffer);
        length = gui_chat_strlen_screen (short_name);
        num_spaces = length_allowed - length;

        if (CONFIG_INTEGER(config_look_prefix_buffer_align) == CONFIG_LOOK_PREFIX_BUFFER_ALIGN_RIGHT)
        {
            if (!simulate)
            {
                gui_chat_reset_style (window, line, 0, 1,
                                      GUI_COLOR_CHAT_INACTIVE_WINDOW,
                                      GUI_COLOR_CHAT_INACTIVE_BUFFER,
                                      GUI_COLOR_CHAT);
            }
            for (i = 0; i < num_spaces; i++)
            {
                gui_chat_display_word (window, line, str_space,
                                       NULL, 1, num_lines, count,
                                       pre_lines_displayed, lines_displayed,
                                       simulate,
                                       CONFIG_BOOLEAN(config_look_color_inactive_prefix_buffer),
                                       0);
            }
        }

        if (!simulate)
        {
            gui_chat_reset_style (window, line, 0, 1,
                                  GUI_COLOR_CHAT_INACTIVE_WINDOW,
                                  (CONFIG_BOOLEAN(config_look_color_inactive_buffer)
                                   && CONFIG_BOOLEAN(config_look_color_inactive_prefix_buffer)
                                   && !line->data->buffer->active) ?
                                  GUI_COLOR_CHAT_PREFIX_BUFFER_INACTIVE_BUFFER :
                                  GUI_COLOR_CHAT_PREFIX_BUFFER,
                                  GUI_COLOR_CHAT_PREFIX_BUFFER);
        }

        if (window->win_chat_cursor_y < window->coords_size)
            window->coords[window->win_chat_cursor_y].buffer_x1 = window->win_chat_cursor_x;

        /* not enough space to display full buffer name? => truncate it! */
        if ((CONFIG_INTEGER(config_look_prefix_buffer_align) != CONFIG_LOOK_PREFIX_BUFFER_ALIGN_NONE)
            && (num_spaces < 0))
        {
            chars_to_display = length_allowed;
            /*
             * if the "+" is not displayed in the space after text, remove one
             * more char to display the "+" before the space
             */
            if (!CONFIG_BOOLEAN(config_look_prefix_buffer_align_more_after))
                chars_to_display--;
            gui_chat_display_word (window, line,
                                   short_name,
                                   short_name +
                                   gui_chat_string_real_pos (short_name,
                                                             chars_to_display,
                                                             1),
                                   1, num_lines, count,
                                   pre_lines_displayed, lines_displayed,
                                   simulate,
                                   CONFIG_BOOLEAN(config_look_color_inactive_prefix_buffer),
                                   0);
        }
        else
        {
            gui_chat_display_word (window, line,
                                   short_name, NULL, 1, num_lines, count,
                                   pre_lines_displayed, lines_displayed,
                                   simulate,
                                   CONFIG_BOOLEAN(config_look_color_inactive_prefix_buffer),
                                   0);
        }

        if (window->win_chat_cursor_y < window->coords_size)
            window->coords[window->win_chat_cursor_y].buffer_x2 = window->win_chat_cursor_x - 1;

        if ((CONFIG_INTEGER(config_look_prefix_buffer_align) != CONFIG_LOOK_PREFIX_BUFFER_ALIGN_NONE)
            && (num_spaces < 0))
        {
            if (!simulate)
            {
                gui_window_set_weechat_color (GUI_WINDOW_OBJECTS(window)->win_chat,
                                              GUI_COLOR_CHAT_PREFIX_MORE);
            }
            gui_chat_display_word (window, line,
                                   CONFIG_STRING(config_look_prefix_buffer_align_more),
                                   NULL, 1, num_lines, count,
                                   pre_lines_displayed, lines_displayed,
                                   simulate,
                                   CONFIG_BOOLEAN(config_look_color_inactive_prefix_buffer),
                                   0);
            if (!CONFIG_BOOLEAN(config_look_prefix_buffer_align_more_after))
            {
                if (!simulate)
                {
                    gui_chat_reset_style (window, line, 0, 1,
                                          GUI_COLOR_CHAT_INACTIVE_WINDOW,
                                          GUI_COLOR_CHAT_INACTIVE_BUFFER,
                                          GUI_COLOR_CHAT);
                }
                gui_chat_display_word (window, line, str_space,
                                       NULL, 1, num_lines, count,
                                       pre_lines_displayed, lines_displayed,
                                       simulate,
                                       CONFIG_BOOLEAN(config_look_color_inactive_prefix),
                                       0);
            }
        }
        else
        {
            if (!simulate)
            {
                gui_chat_reset_style (window, line, 0, 1,
                                      GUI_COLOR_CHAT_INACTIVE_WINDOW,
                                      GUI_COLOR_CHAT_INACTIVE_BUFFER,
                                      GUI_COLOR_CHAT);
            }
            if ((CONFIG_INTEGER(config_look_prefix_buffer_align) == CONFIG_LOOK_PREFIX_BUFFER_ALIGN_LEFT)
                || ((CONFIG_INTEGER(config_look_prefix_buffer_align) == CONFIG_LOOK_PREFIX_BUFFER_ALIGN_NONE)
                && (CONFIG_INTEGER(config_look_prefix_align) != CONFIG_LOOK_PREFIX_ALIGN_NONE)))
            {
                for (i = 0; i < num_spaces; i++)
                {
                    gui_chat_display_word (window, line, str_space,
                                           NULL, 1, num_lines, count,
                                           pre_lines_displayed, lines_displayed,
                                           simulate,
                                           CONFIG_BOOLEAN(config_look_color_inactive_prefix_buffer),
                                           0);
                }
            }
            if (mixed_lines->buffer_max_length > 0)
            {
                gui_chat_display_word (window, line, str_space,
                                       NULL, 1, num_lines, count,
                                       pre_lines_displayed, lines_displayed,
                                       simulate,
                                       CONFIG_BOOLEAN(config_look_color_inactive_prefix_buffer),
                                       0);
            }
        }
    }

    /* get prefix for display */
    gui_line_get_prefix_for_display (line, &ptr_prefix, &prefix_length,
                                     &ptr_prefix_color, &prefix_is_nick);
    if (ptr_prefix)
    {
        ptr_prefix2 = NULL;
        if (ptr_prefix_color && ptr_prefix_color[0])
        {
            str_color = gui_color_get_custom (ptr_prefix_color);
            if (str_color && str_color[0])
            {
                length = strlen (str_color) + strlen (ptr_prefix) + 1;
                ptr_prefix2 = malloc (length);
                if (ptr_prefix2)
                {
                    snprintf (ptr_prefix2, length, "%s%s",
                              str_color, ptr_prefix);
                }
            }
        }
        ptr_prefix = (ptr_prefix2) ? ptr_prefix2 : strdup (ptr_prefix);
    }

    /* get nick prefix/suffix (if prefix is a nick) */
    if (prefix_is_nick && (config_length_nick_prefix_suffix > 0))
    {
        ptr_nick_prefix = CONFIG_STRING(config_look_nick_prefix);
        ptr_nick_suffix = CONFIG_STRING(config_look_nick_suffix);
        length_nick_prefix_suffix = config_length_nick_prefix_suffix;
    }
    else
    {
        ptr_nick_prefix = NULL;
        ptr_nick_suffix = NULL;
        length_nick_prefix_suffix = 0;
    }

    /* display prefix */
    if (ptr_prefix
        && (ptr_prefix[0]
            || (CONFIG_INTEGER(config_look_prefix_align) != CONFIG_LOOK_PREFIX_ALIGN_NONE)))
    {
        if (!simulate)
        {
            gui_chat_reset_style (window, line, 0, 1,
                                  GUI_COLOR_CHAT_INACTIVE_WINDOW,
                                  GUI_COLOR_CHAT_INACTIVE_BUFFER,
                                  GUI_COLOR_CHAT);
        }

        if (CONFIG_INTEGER(config_look_prefix_align_max) > 0)
        {
            length_allowed =
                (window->buffer->lines->prefix_max_length <= CONFIG_INTEGER(config_look_prefix_align_max)) ?
                window->buffer->lines->prefix_max_length : CONFIG_INTEGER(config_look_prefix_align_max);
        }
        else
            length_allowed = window->buffer->lines->prefix_max_length;

        /*
         * if we are not able to display at least 1 char of prefix (inside
         * prefix/suffix), then do not display nick prefix/suffix at all
         */
        if (ptr_nick_prefix && ptr_nick_suffix
            && (length_nick_prefix_suffix + 1 > length_allowed))
        {
            ptr_nick_prefix = NULL;
            ptr_nick_suffix = NULL;
            length_nick_prefix_suffix = 0;
        }

        num_spaces = length_allowed - prefix_length - length_nick_prefix_suffix;

        if (CONFIG_INTEGER(config_look_prefix_align) == CONFIG_LOOK_PREFIX_ALIGN_RIGHT)
        {
            for (i = 0; i < num_spaces; i++)
            {
                gui_chat_display_word (window, line, str_space,
                                       NULL, 1, num_lines, count,
                                       pre_lines_displayed, lines_displayed,
                                       simulate,
                                       CONFIG_BOOLEAN(config_look_color_inactive_prefix),
                                       0);
            }
        }

        /* display prefix before nick (for example "<") */
        if (ptr_nick_prefix)
        {
            if (!simulate)
            {
                gui_window_set_weechat_color (GUI_WINDOW_OBJECTS(window)->win_chat,
                                              GUI_COLOR_CHAT_NICK_PREFIX);
            }
            gui_chat_display_word (window, line,
                                   ptr_nick_prefix,
                                   NULL, 1, num_lines, count,
                                   pre_lines_displayed, lines_displayed,
                                   simulate, 0, 0);
        }

        nick_offline = CONFIG_BOOLEAN(config_look_color_nick_offline)
            && gui_line_has_offline_nick (line);

        prefix_highlighted = NULL;
        if (line->data->highlight)
        {
            prefix_no_color = gui_color_decode (ptr_prefix, NULL);
            if (prefix_no_color)
            {
                length = strlen (prefix_no_color) + 32;
                prefix_highlighted = malloc (length);
                if (prefix_highlighted)
                {
                    snprintf (prefix_highlighted, length, "%s%s",
                              GUI_COLOR(GUI_COLOR_CHAT_HIGHLIGHT),
                              prefix_no_color);
                }
                free (prefix_no_color);
            }
            if (!simulate)
            {
                gui_chat_reset_style (window, line, (nick_offline) ? 2 : 0, 1,
                                      GUI_COLOR_CHAT_INACTIVE_WINDOW,
                                      (CONFIG_BOOLEAN(config_look_color_inactive_buffer)
                                       && CONFIG_BOOLEAN(config_look_color_inactive_prefix)
                                       && !line->data->buffer->active) ?
                                      GUI_COLOR_CHAT_INACTIVE_BUFFER :
                                      GUI_COLOR_CHAT_HIGHLIGHT,
                                      GUI_COLOR_CHAT_HIGHLIGHT);
            }
        }
        else
        {
            if (!simulate)
            {
                gui_chat_reset_style (window, line, nick_offline, 1,
                                      GUI_COLOR_CHAT_INACTIVE_WINDOW,
                                      (CONFIG_BOOLEAN(config_look_color_inactive_buffer)
                                       && CONFIG_BOOLEAN(config_look_color_inactive_prefix)
                                       && !line->data->buffer->active) ?
                                      GUI_COLOR_CHAT_INACTIVE_BUFFER :
                                      GUI_COLOR_CHAT,
                                      GUI_COLOR_CHAT);
            }
        }

        if (window->win_chat_cursor_y < window->coords_size)
            window->coords[window->win_chat_cursor_y].prefix_x1 = window->win_chat_cursor_x;

        /* not enough space to display full prefix? => truncate it! */
        extra_spaces = 0;
        if ((CONFIG_INTEGER(config_look_prefix_align) != CONFIG_LOOK_PREFIX_ALIGN_NONE)
            && (num_spaces < 0))
        {
            chars_to_display = length_allowed - length_nick_prefix_suffix;
            /*
             * if the "+" is not displayed in the space after text, remove one
             * more char to display the "+" before the space
             */
            if (!CONFIG_BOOLEAN(config_look_prefix_align_more_after))
                chars_to_display--;
            chars_displayed = gui_chat_display_word (window, line,
                                                     (prefix_highlighted) ? prefix_highlighted : ptr_prefix,
                                                     (prefix_highlighted) ?
                                                     prefix_highlighted + gui_chat_string_real_pos (prefix_highlighted,
                                                                                                    chars_to_display, 1) :
                                                     ptr_prefix + gui_chat_string_real_pos (ptr_prefix,
                                                                                            chars_to_display, 1),
                                                     1, num_lines, count,
                                                     pre_lines_displayed,
                                                     lines_displayed,
                                                     simulate,
                                                     CONFIG_BOOLEAN(config_look_color_inactive_prefix),
                                                     nick_offline);
            extra_spaces = length_allowed - length_nick_prefix_suffix - 1 - chars_displayed;
        }
        else
        {
            gui_chat_display_word (window, line,
                                   (prefix_highlighted) ? prefix_highlighted : ptr_prefix,
                                   NULL, 1, num_lines, count,
                                   pre_lines_displayed, lines_displayed,
                                   simulate,
                                   CONFIG_BOOLEAN(config_look_color_inactive_prefix),
                                   nick_offline);
        }

        if (window->win_chat_cursor_y < window->coords_size)
            window->coords[window->win_chat_cursor_y].prefix_x2 = window->win_chat_cursor_x - 1;

        if (prefix_highlighted)
            free (prefix_highlighted);

        if (!simulate)
        {
            gui_chat_reset_style (window, line, 0, 1,
                                  GUI_COLOR_CHAT_INACTIVE_WINDOW,
                                  GUI_COLOR_CHAT_INACTIVE_BUFFER,
                                  GUI_COLOR_CHAT);
        }

        for (i = 0; i < extra_spaces; i++)
        {
            gui_chat_display_word (window, line, str_space,
                                   NULL, 1, num_lines, count,
                                   pre_lines_displayed, lines_displayed,
                                   simulate,
                                   CONFIG_BOOLEAN(config_look_color_inactive_prefix),
                                   0);
        }

        if (!CONFIG_BOOLEAN(config_look_prefix_align_more_after)
            && (CONFIG_INTEGER(config_look_prefix_align) != CONFIG_LOOK_PREFIX_ALIGN_NONE)
            && (num_spaces < 0))
        {
            if (!simulate)
            {
                gui_window_set_weechat_color (GUI_WINDOW_OBJECTS(window)->win_chat,
                                              GUI_COLOR_CHAT_PREFIX_MORE);
            }
            gui_chat_display_word (window, line,
                                   CONFIG_STRING(config_look_prefix_align_more),
                                   NULL, 1, num_lines, count,
                                   pre_lines_displayed, lines_displayed,
                                   simulate,
                                   CONFIG_BOOLEAN(config_look_color_inactive_prefix),
                                   0);
        }

        /* display suffix after nick (for example ">") */
        if (ptr_nick_suffix)
        {
            if (!simulate)
            {
                gui_window_set_weechat_color (GUI_WINDOW_OBJECTS(window)->win_chat,
                                              GUI_COLOR_CHAT_NICK_SUFFIX);
            }
            gui_chat_display_word (window, line,
                                   ptr_nick_suffix,
                                   NULL, 1, num_lines, count,
                                   pre_lines_displayed, lines_displayed,
                                   simulate, 0, 0);
        }

        if (CONFIG_INTEGER(config_look_prefix_align) == CONFIG_LOOK_PREFIX_ALIGN_LEFT)
        {
            if (!simulate)
            {
                gui_chat_reset_style (window, line, 0, 1,
                                      GUI_COLOR_CHAT_INACTIVE_WINDOW,
                                      GUI_COLOR_CHAT_INACTIVE_BUFFER,
                                      GUI_COLOR_CHAT);
            }
            for (i = 0; i < num_spaces; i++)
            {
                gui_chat_display_word (window, line, str_space,
                                       NULL, 1, num_lines, count,
                                       pre_lines_displayed, lines_displayed,
                                       simulate,
                                       CONFIG_BOOLEAN(config_look_color_inactive_prefix),
                                       0);
            }
        }

        if (CONFIG_BOOLEAN(config_look_prefix_align_more_after)
            && (CONFIG_INTEGER(config_look_prefix_align) != CONFIG_LOOK_PREFIX_ALIGN_NONE)
            && (num_spaces < 0))
        {
            if (!simulate)
            {
                gui_window_set_weechat_color (GUI_WINDOW_OBJECTS(window)->win_chat,
                                              GUI_COLOR_CHAT_PREFIX_MORE);
            }
            gui_chat_display_word (window, line,
                                   CONFIG_STRING(config_look_prefix_align_more),
                                   NULL, 1, num_lines, count,
                                   pre_lines_displayed, lines_displayed,
                                   simulate,
                                   CONFIG_BOOLEAN(config_look_color_inactive_prefix),
                                   0);
        }
        else
        {
            if (window->buffer->lines->prefix_max_length > 0)
            {
                if (!simulate)
                {
                    gui_chat_reset_style (window, line, 0, 1,
                                          GUI_COLOR_CHAT_INACTIVE_WINDOW,
                                          GUI_COLOR_CHAT_INACTIVE_BUFFER,
                                          GUI_COLOR_CHAT);
                }
                gui_chat_display_word (window, line, str_space,
                                       NULL, 1, num_lines, count,
                                       pre_lines_displayed, lines_displayed,
                                       simulate,
                                       CONFIG_BOOLEAN(config_look_color_inactive_prefix),
                                       0);
            }
        }

        if ((CONFIG_INTEGER(config_look_prefix_align) != CONFIG_LOOK_PREFIX_ALIGN_NONE)
            && CONFIG_STRING(config_look_prefix_suffix)
            && CONFIG_STRING(config_look_prefix_suffix)[0])
        {
            if (!simulate)
            {
                gui_window_set_weechat_color (GUI_WINDOW_OBJECTS(window)->win_chat,
                                              GUI_COLOR_CHAT_PREFIX_SUFFIX);
            }
            gui_chat_display_word (window, line,
                                   CONFIG_STRING(config_look_prefix_suffix),
                                   NULL, 1, num_lines, count,
                                   pre_lines_displayed, lines_displayed,
                                   simulate, 0, 0);
            gui_chat_display_word (window, line, str_space,
                                   NULL, 1, num_lines, count,
                                   pre_lines_displayed, lines_displayed,
                                   simulate, 0, 0);
        }
    }
    if (ptr_prefix)
        free (ptr_prefix);
}

/*
 * Displays a line in the chat window.
 *
 * If count == 0, display whole line.
 * If count > 0, display 'count' lines (beginning from the end).
 * If simulate == 1, nothing is displayed (for counting how many lines would
 * have been displayed).
 *
 * Returns number of lines displayed (or simulated).
 */

int
gui_chat_display_line (struct t_gui_window *window, struct t_gui_line *line,
                       int count, int simulate)
{
    int num_lines, x, y, pre_lines_displayed, lines_displayed, line_align;
    int read_marker_x, read_marker_y;
    int word_start_offset, word_end_offset;
    int word_length_with_spaces, word_length;
    char *message_with_tags, *message_with_search;
    const char *ptr_data, *ptr_end_offset, *ptr_style, *next_char;
    struct t_gui_line *ptr_prev_line, *ptr_next_line;
    struct tm local_time, local_time2;
    struct timeval tv_time;
    time_t seconds, *ptr_time;

    if (!line)
        return 0;

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
        gui_window_current_emphasis = 0;
    }

    pre_lines_displayed = 0;
    lines_displayed = 0;

    /* display message before first line of buffer if date is not today */
    if ((line->data->date != 0)
        && CONFIG_BOOLEAN(config_look_day_change)
        && window->buffer->day_change)
    {
        ptr_time = NULL;
        ptr_prev_line = gui_line_get_prev_displayed (line);
        if (ptr_prev_line)
        {
            while (ptr_prev_line && (ptr_prev_line->data->date == 0))
            {
                ptr_prev_line = gui_line_get_prev_displayed (ptr_prev_line);
            }
        }
        if (!ptr_prev_line)
        {
            gettimeofday (&tv_time, NULL);
            seconds = tv_time.tv_sec;
            localtime_r (&seconds, &local_time);
            localtime_r (&line->data->date, &local_time2);
            if ((local_time.tm_mday != local_time2.tm_mday)
                || (local_time.tm_mon != local_time2.tm_mon)
                || (local_time.tm_year != local_time2.tm_year))
            {
                gui_chat_display_day_changed (window, NULL, &local_time2,
                                              simulate);
                gui_chat_display_new_line (window, num_lines, count,
                                           &lines_displayed, simulate);
                pre_lines_displayed++;
            }
        }
    }

    /* calculate marker position (maybe not used for this line!) */
    if (window->buffer->time_for_each_line && line->data->str_time)
        read_marker_x = x + gui_chat_strlen_screen (line->data->str_time);
    else
        read_marker_x = x;
    read_marker_y = y;

    /* display time and prefix */
    gui_chat_display_time_to_prefix (window, line, num_lines, count,
                                     pre_lines_displayed, &lines_displayed,
                                     simulate);
    if (!simulate && !gui_chat_display_tags)
    {
        if (window->win_chat_cursor_y < window->coords_size)
            window->coords[window->win_chat_cursor_y].data = line->data->message;
    }

    /* reset color & style for a new line */
    if (!simulate)
    {
        if (CONFIG_BOOLEAN(config_look_color_inactive_message))
        {
            gui_chat_reset_style (window, line, 0, 1,
                                  GUI_COLOR_CHAT_INACTIVE_WINDOW,
                                  GUI_COLOR_CHAT_INACTIVE_BUFFER,
                                  GUI_COLOR_CHAT);
        }
        else
        {
            gui_chat_reset_style (window, line, 0, 1,
                                  GUI_COLOR_CHAT,
                                  GUI_COLOR_CHAT,
                                  GUI_COLOR_CHAT);
        }
    }

    /* display message */
    ptr_data = NULL;
    message_with_tags = NULL;
    message_with_search = NULL;

    if (line->data->message && line->data->message[0])
    {
        message_with_tags = (gui_chat_display_tags) ?
            gui_line_build_string_message_tags (line->data->message,
                                                line->data->tags_count,
                                                line->data->tags_array,
                                                1) : NULL;
        ptr_data = (message_with_tags) ?
            message_with_tags : line->data->message;
        message_with_search = NULL;
        if ((window->buffer->text_search != GUI_TEXT_SEARCH_DISABLED)
            && (window->buffer->text_search_where & GUI_TEXT_SEARCH_IN_MESSAGE)
            && (!window->buffer->text_search_regex
                || window->buffer->text_search_regex_compiled))
        {
            message_with_search = gui_color_emphasize (ptr_data,
                                                       window->buffer->input_buffer,
                                                       window->buffer->text_search_exact,
                                                       window->buffer->text_search_regex_compiled);
            if (message_with_search)
                ptr_data = message_with_search;
        }
    }

    if (ptr_data && ptr_data[0])
    {
        while (ptr_data && ptr_data[0])
        {
            if (ptr_data[0] == '\n')
            {
                gui_chat_display_new_line (window, num_lines, count,
                                           &lines_displayed, simulate);
                ptr_data++;
                gui_chat_display_prefix_suffix(window, line,
                                               ptr_data,
                                               pre_lines_displayed, &lines_displayed,
                                               0,
                                               simulate,
                                               CONFIG_BOOLEAN(config_look_color_inactive_message),
                                               0);
            }

            gui_chat_get_word_info (window,
                                    ptr_data,
                                    &word_start_offset,
                                    &word_end_offset,
                                    &word_length_with_spaces, &word_length);

            ptr_end_offset = ptr_data + word_end_offset;

            /* if message ends with spaces, display them */
            if ((word_length <= 0) && (word_length_with_spaces > 0)
                && !ptr_data[word_end_offset])
            {
                word_length = word_length_with_spaces;
            }

            if (word_length >= 0)
            {
                line_align = gui_line_get_align (window->buffer, line, 1,
                                                 (lines_displayed == 0) ? 1 : 0);
                if ((window->win_chat_cursor_x + word_length_with_spaces > gui_chat_get_real_width (window))
                    && (word_length <= gui_chat_get_real_width (window) - line_align))
                {
                    /* spaces + word too long for current line but OK for next line */
                    gui_chat_display_new_line (window, num_lines, count,
                                               &lines_displayed, simulate);
                    /* apply styles before jumping to start of word */
                    if (!simulate && (word_start_offset > 0))
                    {
                        ptr_style = ptr_data;
                        while (ptr_style < ptr_data + word_start_offset)
                        {
                            /* loop until no style/char available */
                            ptr_style = gui_chat_string_next_char (window, line,
                                                                   (unsigned char *)ptr_style,
                                                                   1,
                                                                   CONFIG_BOOLEAN(config_look_color_inactive_message),
                                                                   0);
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
                                       0, num_lines, count,
                                       pre_lines_displayed, &lines_displayed,
                                       simulate,
                                       CONFIG_BOOLEAN(config_look_color_inactive_message),
                                       0);

                if ((!simulate) && (window->win_chat_cursor_y >= window->win_chat_height))
                    ptr_data = NULL;
                else
                {
                    /* move pointer after end of word */
                    ptr_data = ptr_end_offset;
                    if (*(ptr_data - 1) == '\0')
                        ptr_data = NULL;

                    if (window->win_chat_cursor_x == 0)
                    {
                        while (ptr_data && (ptr_data[0] == ' '))
                        {
                            next_char = utf8_next_char (ptr_data);
                            if (!next_char)
                                break;
                            ptr_data = gui_chat_string_next_char (window, line,
                                                                  (unsigned char *)next_char,
                                                                  1,
                                                                  CONFIG_BOOLEAN(config_look_color_inactive_message),
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
    else
    {
        /* no message */
        gui_chat_display_new_line (window, num_lines, count,
                                   &lines_displayed, simulate);
    }

    if (message_with_tags)
        free (message_with_tags);
    if (message_with_search)
        free (message_with_search);

    /* display message if day has changed after this line */
    if ((line->data->date != 0)
        && CONFIG_BOOLEAN(config_look_day_change)
        && window->buffer->day_change)
    {
        ptr_time = NULL;
        ptr_next_line = gui_line_get_next_displayed (line);
        if (ptr_next_line)
        {
            while (ptr_next_line && (ptr_next_line->data->date == 0))
            {
                ptr_next_line = gui_line_get_next_displayed (ptr_next_line);
            }
        }
        if (ptr_next_line)
        {
            /* get time of next line */
            ptr_time = &ptr_next_line->data->date;
        }
        else
        {
            /* it was the last line => compare with current system time */
            gettimeofday (&tv_time, NULL);
            seconds = tv_time.tv_sec;
            ptr_time = &seconds;
        }
        if (ptr_time && (*ptr_time != 0))
        {
            localtime_r (&line->data->date, &local_time);
            localtime_r (ptr_time, &local_time2);
            if ((local_time.tm_mday != local_time2.tm_mday)
                || (local_time.tm_mon != local_time2.tm_mon)
                || (local_time.tm_year != local_time2.tm_year))
            {
                gui_chat_display_day_changed (window, &local_time, &local_time2,
                                              simulate);
                gui_chat_display_new_line (window, num_lines, count,
                                           &lines_displayed, simulate);
            }
        }
    }

    /* display read marker (after line) */
    if (gui_chat_marker_for_line (window->buffer, line))
    {
        gui_chat_display_horizontal_line (window, simulate);
        gui_chat_display_new_line (window, num_lines, count,
                                   &lines_displayed, simulate);
    }

    if (simulate)
    {
        window->win_chat_cursor_x = x;
        window->win_chat_cursor_y = y;
    }
    else
    {
        /* display marker if line is matching user search */
        if (window->buffer->text_search != GUI_TEXT_SEARCH_DISABLED)
        {
            if (gui_line_search_text (window->buffer, line))
            {
                gui_window_set_weechat_color (GUI_WINDOW_OBJECTS(window)->win_chat,
                                              GUI_COLOR_CHAT_TEXT_FOUND);
                mvwaddstr (GUI_WINDOW_OBJECTS(window)->win_chat,
                           read_marker_y, read_marker_x,
                           "*");
            }
        }
        else
        {
            /* display read marker if needed */
            if ((CONFIG_INTEGER(config_look_read_marker) == CONFIG_LOOK_READ_MARKER_CHAR)
                && window->buffer->lines->last_read_line
                && (window->buffer->lines->last_read_line == gui_line_get_prev_displayed (line)))
            {
                gui_window_set_weechat_color (GUI_WINDOW_OBJECTS(window)->win_chat,
                                              GUI_COLOR_CHAT_READ_MARKER);
                mvwaddstr (GUI_WINDOW_OBJECTS(window)->win_chat,
                           read_marker_y, read_marker_x,
                           "*");
            }
        }
    }

    return lines_displayed;
}

/*
 * Displays a line in the chat window (for a buffer with free content).
 */

void
gui_chat_display_line_y (struct t_gui_window *window, struct t_gui_line *line,
                         int y)
{
    char *ptr_data, *message_with_tags, *message_with_search;

    message_with_tags = NULL;
    message_with_search = NULL;

    /* reset color & style for a new line */
    gui_chat_reset_style (window, line, 0, 1,
                          GUI_COLOR_CHAT_INACTIVE_WINDOW,
                          GUI_COLOR_CHAT_INACTIVE_BUFFER,
                          GUI_COLOR_CHAT);

    window->win_chat_cursor_x = 0;
    window->win_chat_cursor_y = y;
    gui_window_current_emphasis = 0;

    gui_chat_clrtoeol (window);

    ptr_data = line->data->message;

    /* add tags if debug of tags is enabled */
    if (gui_chat_display_tags)
    {
        message_with_tags = gui_line_build_string_message_tags (
            ptr_data,
            line->data->tags_count,
            line->data->tags_array,
            1);
        if (message_with_tags)
            ptr_data = message_with_tags;
    }

    /* emphasize text (if searching text) */
    if ((window->buffer->text_search != GUI_TEXT_SEARCH_DISABLED)
        && (window->buffer->text_search_where & GUI_TEXT_SEARCH_IN_MESSAGE)
        && (!window->buffer->text_search_regex
            || window->buffer->text_search_regex_compiled))
    {
        message_with_search = gui_color_emphasize (ptr_data,
                                                   window->buffer->input_buffer,
                                                   window->buffer->text_search_exact,
                                                   window->buffer->text_search_regex_compiled);
        if (message_with_search)
            ptr_data = message_with_search;
    }

    /* display the line */
    if (gui_chat_display_word_raw (window, line, ptr_data,
                                   window->win_chat_width, 0,
                                   CONFIG_BOOLEAN(config_look_color_inactive_message),
                                   0) < window->win_chat_width)
    {
        gui_window_clrtoeol (GUI_WINDOW_OBJECTS(window)->win_chat);
    }

    if (message_with_tags)
        free (message_with_tags);
    if (message_with_search)
        free (message_with_search);
}

/*
 * Returns pointer to line & offset for a difference with given line.
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

    if (*line && (*line_pos < 0))
    {
        *line = (*line)->next_line;
        *line_pos = 0;
    }

    if (!(*line))
    {
        /* if looking backward, start at last line of buffer */
        if (backward)
        {
            *line = gui_line_get_last_displayed (window->buffer);
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
            *line = gui_line_get_first_displayed (window->buffer);
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
                *line = gui_line_get_prev_displayed (*line);
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
                *line = gui_line_get_next_displayed (*line);
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
            *line = gui_line_get_first_displayed (window->buffer);
            *line_pos = 0;
        }
        else
        {
            /* last line reached => consider we'll display all until the end */
            *line_pos = 0;
        }
    }

    /* special case for bare display */
    if (gui_window_bare_display && backward && (*line_pos > 0))
    {
        *line = gui_line_get_next_displayed (*line);
        *line_pos = 0;
    }
}

/*
 * Draws chat window for a formatted buffer.
 */

void
gui_chat_draw_formatted_buffer (struct t_gui_window *window)
{
    struct t_gui_line *ptr_line, *ptr_line2;
    int auto_search_first_line, line_pos, line_pos2, count;
    int old_scrolling, old_lines_after;

    /* display at position of scrolling */
    auto_search_first_line = 1;
    ptr_line = NULL;
    line_pos = 0;
    if (window->scroll->start_line)
    {
        auto_search_first_line = 0;
        ptr_line = window->scroll->start_line;
        line_pos = window->scroll->start_line_pos;
        if (line_pos < 0)
        {
            ptr_line = gui_line_get_next_displayed (ptr_line);
            line_pos = 0;
            if (ptr_line)
            {
                ptr_line2 = ptr_line;
                line_pos2 = 0;
                gui_chat_calculate_line_diff (window, &ptr_line2, &line_pos2,
                                              window->win_chat_height);
                if (ptr_line2)
                {
                    auto_search_first_line = 1;
                    window->scroll->start_line = NULL;
                    window->scroll->start_line_pos = 0;
                    ptr_line = NULL;
                    line_pos = 0;
                }
            }
        }
        else if (!ptr_line->data->displayed)
        {
            /* skip filtered lines on top when scrolling */
            ptr_line = gui_line_get_next_displayed (ptr_line);
            line_pos = 0;
        }
    }

    if (auto_search_first_line)
    {
        /* look for first line to display, starting from last line */
        gui_chat_calculate_line_diff (window, &ptr_line, &line_pos,
                                      (-1) * (window->win_chat_height - 1));
    }

    if (!ptr_line)
        return;

    count = 0;

    if (line_pos > 0)
    {
        /* display end of first line at top of screen */
        count = gui_chat_display_line (window, ptr_line,
                                       gui_chat_display_line (window,
                                                              ptr_line,
                                                              0, 1) -
                                       line_pos, 0);
        ptr_line = gui_line_get_next_displayed (ptr_line);
        window->scroll->first_line_displayed = 0;
    }
    else
        window->scroll->first_line_displayed =
            (ptr_line == gui_line_get_first_displayed (window->buffer));

    /* display lines */
    while (ptr_line && (window->win_chat_cursor_y <= window->win_chat_height - 1))
    {
        count = gui_chat_display_line (window, ptr_line, 0, 0);
        ptr_line = gui_line_get_next_displayed (ptr_line);
    }

    old_scrolling = window->scroll->scrolling;
    old_lines_after = window->scroll->lines_after;

    window->scroll->scrolling = (window->win_chat_cursor_y > window->win_chat_height - 1);

    /* check if last line of buffer is entirely displayed and scrolling */
    /* if so, disable scroll indicator */
    if (!ptr_line && window->scroll->scrolling)
    {
        if ((count == gui_chat_display_line (window, gui_line_get_last_displayed (window->buffer), 0, 1))
            || (count == window->win_chat_height))
            window->scroll->scrolling = 0;
    }

    if (!window->scroll->scrolling
        && (window->scroll->start_line == gui_line_get_first_displayed (window->buffer)))
    {
        window->scroll->start_line = NULL;
        window->scroll->start_line_pos = 0;
    }

    window->scroll->lines_after = 0;
    if (window->scroll->scrolling && ptr_line)
    {
        /* count number of lines after last line displayed */
        while (ptr_line)
        {
            ptr_line = gui_line_get_next_displayed (ptr_line);
            if (ptr_line)
                window->scroll->lines_after++;
        }
        window->scroll->lines_after++;
    }

    if ((window->scroll->scrolling != old_scrolling)
        || (window->scroll->lines_after != old_lines_after))
    {
        (void) hook_signal_send ("window_scrolled",
                                 WEECHAT_HOOK_SIGNAL_POINTER, window);
    }

    /* cursor is below end line of chat window? */
    if (window->win_chat_cursor_y > window->win_chat_height - 1)
    {
        window->win_chat_cursor_x = 0;
        window->win_chat_cursor_y = window->win_chat_height - 1;
    }
}

/*
 * Draws chat window for a free buffer.
 */

void
gui_chat_draw_free_buffer (struct t_gui_window *window, int clear_chat)
{
    struct t_gui_line *ptr_line;
    int y_start, y_end, y;

    ptr_line = NULL;
    if (window->scroll->start_line)
    {
        ptr_line = window->scroll->start_line;
        if (window->scroll->start_line_pos < 0)
            ptr_line = gui_line_get_next_displayed (ptr_line);
    }
    else
        ptr_line = window->buffer->lines->first_line;

    if (ptr_line)
    {
        if (!ptr_line->data->displayed)
            ptr_line = gui_line_get_next_displayed (ptr_line);
        if (ptr_line)
        {
            y_start = (window->scroll->start_line) ? ptr_line->data->y : 0;
            y_end = y_start + window->win_chat_height - 1;
            while (ptr_line && (ptr_line->data->y <= y_end))
            {
                y = ptr_line->data->y - y_start;
                if (y < window->coords_size)
                {
                    window->coords[y].line = ptr_line;
                    window->coords[y].data = ptr_line->data->message;
                }
                if (ptr_line->data->refresh_needed || clear_chat)
                {
                    gui_chat_display_line_y (window, ptr_line,
                                             y);
                }
                ptr_line = gui_line_get_next_displayed (ptr_line);
            }
        }
    }
}

/*
 * Gets line content in bare display.
 *
 * Note: result must be freed after use.
 */

char *
gui_chat_get_bare_line (struct t_gui_line *line)
{
    char *prefix, *message, str_time[256], *str_line;
    const char *tag_prefix_nick;
    struct tm *local_time;
    int length;

    prefix = NULL;
    message = NULL;
    str_line = NULL;

    prefix = (line->data->prefix) ?
        gui_color_decode (line->data->prefix, NULL) : strdup ("");
    if (!prefix)
        goto end;
    message = (line->data->message) ?
        gui_color_decode (line->data->message, NULL) : strdup ("");
    if (!message)
        goto end;

    str_time[0] = '\0';
    if (line->data->buffer->time_for_each_line
        && (line->data->date > 0)
        && CONFIG_STRING(config_look_bare_display_time_format)
        && CONFIG_STRING(config_look_bare_display_time_format)[0])
    {
        local_time = localtime (&line->data->date);
        if (strftime (str_time, sizeof (str_time),
                      CONFIG_STRING(config_look_bare_display_time_format),
                      local_time) == 0)
            str_time[0] = '\0';
    }
    tag_prefix_nick = gui_line_search_tag_starting_with (line, "prefix_nick_");

    length = strlen (str_time) + 1 + 1 + strlen (prefix) + 1 + 1
        + strlen (message) + 1;
    str_line = malloc (length);
    if (str_line)
    {
        snprintf (str_line, length,
                  "%s%s%s%s%s%s%s",
                  str_time,
                  (str_time[0]) ? " " : "",
                  (prefix[0] && tag_prefix_nick) ? "<" : "",
                  prefix,
                  (prefix[0] && tag_prefix_nick) ? ">" : "",
                  (prefix[0]) ? " " : "",
                  message);
    }

end:
    if (prefix)
        free (prefix);
    if (message)
        free (message);

    return str_line;
}

/*
 * Draws a buffer in bare display (not ncurses).
 */

void
gui_chat_draw_bare (struct t_gui_window *window)
{
    struct t_gui_line *ptr_gui_line;
    char *line, *ptr_line_start, *ptr_line_end;
    int y, length, num_lines;

    /* in bare display, we display ONLY the current window/buffer */
    if (window != gui_current_window)
        return;

    /* clear screen */
    printf ("\033[2J");

    /* display lines */
    if ((window->buffer->type == GUI_BUFFER_TYPE_FREE)
        || window->scroll->start_line)
    {
        /* display from top to bottom (starting with "start_line") */
        y = 0;
        if (window->scroll->start_line)
        {
            ptr_gui_line = window->scroll->start_line;
            window->scroll->first_line_displayed =
                (ptr_gui_line == gui_line_get_first_displayed (window->buffer));
        }
        else
        {
            ptr_gui_line = gui_line_get_first_displayed (window->buffer);
            window->scroll->first_line_displayed = 1;
        }
        while (ptr_gui_line && (y < gui_term_lines))
        {
            line = gui_chat_get_bare_line (ptr_gui_line);
            if (!line)
                break;

            ptr_line_start = line;
            ptr_line_end = line;
            while (ptr_line_start)
            {
                ptr_line_end = strchr (ptr_line_start, '\n');
                if (ptr_line_end)
                    ptr_line_end[0] = '\0';

                length = utf8_strlen_screen (ptr_line_start);
                num_lines = length == 0 ? 1 : length / gui_term_cols;
                if (length % gui_term_cols != 0)
                    num_lines++;
                if (y + num_lines <= gui_term_lines)
                    printf ("\033[%d;1H%s", y + 1, ptr_line_start);
                y += num_lines;

                if (ptr_line_end)
                {
                    ptr_line_end[0] = '\n';
                    ptr_line_start = ptr_line_end + 1;
                }
                else
                {
                    ptr_line_start = NULL;
                }
            }

            free (line);
            ptr_gui_line = gui_line_get_next_displayed (ptr_gui_line);
        }
    }
    else
    {
        /* display from bottom to top (starting with last line of buffer) */
        y = gui_term_lines;
        ptr_gui_line = gui_line_get_last_displayed (window->buffer);
        while (ptr_gui_line && (y >= 0))
        {
            line = gui_chat_get_bare_line (ptr_gui_line);
            if (!line)
                break;

            ptr_line_start = line;
            ptr_line_end = line;
            while (ptr_line_start)
            {
                ptr_line_end = strchr (ptr_line_start, '\n');
                if (ptr_line_end)
                    ptr_line_end[0] = '\0';
                length = utf8_strlen_screen (ptr_line_start);
                num_lines = length == 0 ? 1 : length / gui_term_cols;
                if (length % gui_term_cols != 0)
                    num_lines++;
                y -= num_lines;
                if (ptr_line_end)
                {
                    ptr_line_end[0] = '\n';
                    ptr_line_start = ptr_line_end + 1;
                }
                else
                {
                    ptr_line_start = NULL;
                }
            }

            if (y >= 0)
            {
                printf ("\033[%d;1H", y + 1);
                ptr_line_start = line;
                ptr_line_end = line;
                while (ptr_line_start)
                {
                    ptr_line_end = strchr (ptr_line_start, '\n');
                    if (ptr_line_end)
                        ptr_line_end[0] = '\0';

                    printf ("%s", ptr_line_start);

                    if (ptr_line_end)
                    {
                        printf ("\r\n");
                        ptr_line_end[0] = '\n';
                        ptr_line_start = ptr_line_end + 1;
                    }
                    else
                    {
                        ptr_line_start = NULL;
                    }
                }
            }

            free (line);
            ptr_gui_line = gui_line_get_prev_displayed (ptr_gui_line);
        }
        window->scroll->first_line_displayed =
            (ptr_gui_line == gui_line_get_first_displayed (window->buffer));
    }

    /*
     * move cursor to top/left or bottom/right, according to type of buffer and
     * whether we are scrolling or not
     */
    printf ("\033[%d;1H",
            (window->buffer->type == GUI_BUFFER_TYPE_FREE) ?
            ((window->scroll->start_line) ? gui_term_lines : 1) :
            ((window->scroll->start_line) ? 1 : gui_term_lines));

    fflush (stdout);
}

/*
 * Draws chat window for a buffer.
 */

void
gui_chat_draw (struct t_gui_buffer *buffer, int clear_chat)
{
    struct t_gui_window *ptr_win;
    struct t_gui_line *ptr_line;
    char format_empty[32];
    int i;

    if (!gui_init_ok)
        return;

    if (gui_window_bare_display)
    {
        if (gui_current_window && (gui_current_window->buffer == buffer))
            gui_chat_draw_bare (gui_current_window);
        goto end;
    }

    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if ((ptr_win->buffer->number == buffer->number)
            && (ptr_win->win_chat_x >= 0) && (ptr_win->win_chat_y >= 0)
            && (GUI_WINDOW_OBJECTS(ptr_win)->win_chat))
        {
            gui_window_coords_alloc (ptr_win);

            gui_chat_reset_style (ptr_win, NULL, 0, 1,
                                  GUI_COLOR_CHAT_INACTIVE_WINDOW,
                                  GUI_COLOR_CHAT_INACTIVE_BUFFER,
                                  GUI_COLOR_CHAT);

            if (clear_chat)
            {
                snprintf (format_empty, sizeof (format_empty),
                          "%%-%ds", ptr_win->win_chat_width);
                for (i = 0; i < ptr_win->win_chat_height; i++)
                {
                    mvwprintw (GUI_WINDOW_OBJECTS(ptr_win)->win_chat, i, 0,
                               format_empty, " ");
                }
            }

            ptr_win->win_chat_cursor_x = 0;
            ptr_win->win_chat_cursor_y = 0;

            switch (ptr_win->buffer->type)
            {
                case GUI_BUFFER_TYPE_FORMATTED:
                    /* min 2 lines for chat area */
                    if (ptr_win->win_chat_height < 2)
                        mvwaddstr (GUI_WINDOW_OBJECTS(ptr_win)->win_chat, 0, 0, "...");
                    else
                        gui_chat_draw_formatted_buffer (ptr_win);
                    break;
                case GUI_BUFFER_TYPE_FREE:
                    gui_chat_draw_free_buffer (ptr_win, clear_chat);
                    break;
                case GUI_BUFFER_NUM_TYPES:
                    break;
            }
            wnoutrefresh (GUI_WINDOW_OBJECTS(ptr_win)->win_chat);
        }
    }

    refresh ();

    if (buffer->type == GUI_BUFFER_TYPE_FREE)
    {
        for (ptr_line = buffer->lines->first_line; ptr_line;
             ptr_line = ptr_line->next_line)
        {
            ptr_line->data->refresh_needed = 0;
        }
    }

end:
    buffer->chat_refresh_needed = 0;
}
