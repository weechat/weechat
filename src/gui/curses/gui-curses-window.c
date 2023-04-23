/*
 * gui-curses-window.c - window display functions for Curses GUI
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
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <libgen.h>
#include <sys/ioctl.h>
#include <termios.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-eval.h"
#include "../../core/wee-hook.h"
#include "../../core/wee-log.h"
#include "../../core/wee-string.h"
#include "../../plugins/plugin.h"
#include "../gui-window.h"
#include "../gui-bar.h"
#include "../gui-bar-window.h"
#include "../gui-buffer.h"
#include "../gui-chat.h"
#include "../gui-color.h"
#include "../gui-cursor.h"
#include "../gui-hotlist.h"
#include "../gui-key.h"
#include "../gui-layout.h"
#include "../gui-line.h"
#include "../gui-main.h"
#include "../gui-mouse.h"
#include "../gui-nicklist.h"
#include "gui-curses.h"
#include "gui-curses-chat.h"
#include "gui-curses-color.h"
#include "gui-curses-main.h"
#include "gui-curses-window.h"


#define GUI_WINDOW_MAX_SAVED_STYLES 32


int gui_window_current_style_fg;       /* current foreground color          */
int gui_window_current_style_bg;       /* current background color          */
int gui_window_current_color_attr;     /* attr sum of last color(s) used    */
int gui_window_current_emphasis;       /* 1 if text emphasis is enabled     */
struct t_gui_window_saved_style gui_window_saved_style[GUI_WINDOW_MAX_SAVED_STYLES];
                                       /* circular list of saved styles     */
int gui_window_saved_style_index = 0;  /* index in list of savec styles     */


/*
 * Gets screen width (terminal width in chars for Curses).
 */

int
gui_window_get_width ()
{
    return gui_term_cols;
}

/*
 * Gets screen height (terminal height in chars for Curses).
 */

int
gui_window_get_height ()
{
    return gui_term_lines;
}

/*
 * Reads terminal size.
 */

void
gui_window_read_terminal_size ()
{
    struct winsize size;
    int new_width, new_height;

    if (ioctl (fileno (stdout), TIOCGWINSZ, &size) == 0)
    {
        resizeterm (size.ws_row, size.ws_col);
        gui_term_cols = size.ws_col;
        gui_term_lines = size.ws_row;
    }
    else
    {
        getmaxyx (stdscr, new_height, new_width);
        gui_term_cols = new_width;
        gui_term_lines = new_height;
    }
}

/*
 * Initializes Curses windows.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
gui_window_objects_init (struct t_gui_window *window)
{
    struct t_gui_window_curses_objects *new_objects;

    new_objects = malloc (sizeof (*new_objects));
    if (new_objects)
    {
        window->gui_objects = new_objects;
        GUI_WINDOW_OBJECTS(window)->win_chat = NULL;
        GUI_WINDOW_OBJECTS(window)->win_separator_horiz = NULL;
        GUI_WINDOW_OBJECTS(window)->win_separator_vertic = NULL;
        return 1;
    }
    return 0;
}

/*
 * Frees Curses windows for a window.
 */

void
gui_window_objects_free (struct t_gui_window *window, int free_separators)
{
    if (!window)
        return;

    if (GUI_WINDOW_OBJECTS(window)->win_chat)
    {
        delwin (GUI_WINDOW_OBJECTS(window)->win_chat);
        GUI_WINDOW_OBJECTS(window)->win_chat = NULL;
    }
    if (free_separators)
    {
        if  (GUI_WINDOW_OBJECTS(window)->win_separator_horiz)
        {
            delwin (GUI_WINDOW_OBJECTS(window)->win_separator_horiz);
            GUI_WINDOW_OBJECTS(window)->win_separator_horiz = NULL;
        }
        if  (GUI_WINDOW_OBJECTS(window)->win_separator_vertic)
        {
            delwin (GUI_WINDOW_OBJECTS(window)->win_separator_vertic);
            GUI_WINDOW_OBJECTS(window)->win_separator_vertic = NULL;
        }
    }
}

/*
 * Clears a Curses window.
 */

void
gui_window_clear (WINDOW *window, int fg, int bg)
{
    int attrs;

    if (!gui_init_ok)
        return;

    attrs = gui_color_get_gui_attrs (fg);

    if ((fg > 0) && (fg & GUI_COLOR_EXTENDED_FLAG))
        fg &= GUI_COLOR_EXTENDED_MASK;
    else
        fg = gui_weechat_colors[fg & GUI_COLOR_EXTENDED_MASK].foreground;

    if ((bg > 0) && (bg & GUI_COLOR_EXTENDED_FLAG))
        bg &= GUI_COLOR_EXTENDED_MASK;
    else
        bg = gui_weechat_colors[bg & GUI_COLOR_EXTENDED_MASK].background;

#ifdef NCURSES_EXT_COLORS
    cchar_t c;
    setcchar (&c, L" ", attrs, gui_color_get_pair (fg, bg), NULL);
    wbkgrndset (window, &c);
#else
    wbkgdset (window, ' ' | COLOR_PAIR (gui_color_get_pair (fg, bg)) | attrs);
#endif
    werase (window);
    wmove (window, 0, 0);
}

/*
 * Clears until end of line with current background.
 */

void
gui_window_clrtoeol (WINDOW *window)
{
#ifdef NCURSES_EXT_COLORS
    cchar_t c;
    setcchar (&c, L" ", A_NORMAL, gui_color_get_pair (gui_window_current_style_fg,
                                                      gui_window_current_style_bg), NULL);
    wbkgrndset (window, &c);
#else
    wbkgdset (window,
              ' ' | COLOR_PAIR (gui_color_get_pair (gui_window_current_style_fg,
                                                    gui_window_current_style_bg)));
#endif
    wclrtoeol (window);
}

/*
 * Saves current style.
 */

void
gui_window_save_style (WINDOW *window)
{
    struct t_gui_window_saved_style *ptr_saved_style;
    attr_t *ptr_attrs;
    short *ptr_pair;

    /* get pointer on saved style */
    ptr_saved_style = &gui_window_saved_style[gui_window_saved_style_index];

    /* save current style */
    ptr_saved_style->style_fg = gui_window_current_style_fg;
    ptr_saved_style->style_bg = gui_window_current_style_bg;
    ptr_saved_style->color_attr = gui_window_current_color_attr;
    ptr_saved_style->emphasis = gui_window_current_emphasis;
    ptr_attrs = &ptr_saved_style->attrs;
    ptr_pair = &ptr_saved_style->pair;
    wattr_get (window, ptr_attrs, ptr_pair, NULL);

    /* increment style index (circular list) */
    gui_window_saved_style_index++;
    if (gui_window_saved_style_index >= GUI_WINDOW_MAX_SAVED_STYLES)
        gui_window_saved_style_index = 0;
}

/*
 * Restores style values.
 */

void
gui_window_restore_style (WINDOW *window)
{
    struct t_gui_window_saved_style *ptr_saved_style;

    /* decrement style index (circular list) */
    gui_window_saved_style_index--;
    if (gui_window_saved_style_index < 0)
        gui_window_saved_style_index = GUI_WINDOW_MAX_SAVED_STYLES - 1;

    /* get pointer on saved style */
    ptr_saved_style = &gui_window_saved_style[gui_window_saved_style_index];

    /* restore style */
    gui_window_current_style_fg = ptr_saved_style->style_fg;
    gui_window_current_style_bg = ptr_saved_style->style_bg;
    gui_window_current_color_attr = ptr_saved_style->color_attr;
    gui_window_current_emphasis = ptr_saved_style->emphasis;
    wattr_set (window, ptr_saved_style->attrs, ptr_saved_style->pair, NULL);
    /*
     * for unknown reason, the wattr_set function sometimes
     * fails to set the color pair under FreeBSD, so we force
     * it again with wcolor_set
     */
    wcolor_set (window, ptr_saved_style->pair, NULL);
}

/*
 * Resets style (color and attr) with a WeeChat color for a window.
 */

void
gui_window_reset_style (WINDOW *window, int weechat_color)
{
    gui_window_current_style_fg = -1;
    gui_window_current_style_bg = -1;
    gui_window_current_color_attr = 0;

    wattr_set (window, gui_color[weechat_color]->attributes,
               gui_color_weechat_get_pair (weechat_color), NULL);
}

/*
 * Resets color with a WeeChat color for a window.
 */

void
gui_window_reset_color (WINDOW *window, int weechat_color)
{
    gui_window_current_style_fg = gui_color[weechat_color]->foreground;
    gui_window_current_style_bg = gui_color[weechat_color]->background;

    wattron (window, gui_color[weechat_color]->attributes);
    wcolor_set (window, gui_color_weechat_get_pair (weechat_color), NULL);
}

/*
 * Sets style for color.
 */

void
gui_window_set_color_style (WINDOW *window, int style)
{
    gui_window_current_color_attr |= style;
    wattron (window, style);
}

/*
 * Removes style for color.
 */

void
gui_window_remove_color_style (WINDOW *window, int style)
{
    gui_window_current_color_attr &= !style;
    wattroff (window, style);
}

/*
 * Sets color for a window.
 */

void
gui_window_set_color (WINDOW *window, int fg, int bg)
{
    gui_window_current_style_fg = fg;
    gui_window_current_style_bg = bg;

    wcolor_set (window, gui_color_get_pair (fg, bg), NULL);
}

/*
 * Sets WeeChat color for window.
 */

void
gui_window_set_weechat_color (WINDOW *window, int num_color)
{
    int fg, bg;

    if ((num_color >= 0) && (num_color < GUI_COLOR_NUM_COLORS))
    {
        gui_window_reset_style (window, num_color);
        fg = gui_color[num_color]->foreground;
        bg = gui_color[num_color]->background;

        /*
         * if not real white, we use default terminal foreground instead of
         * white if bold attribute is set
         */
        if ((fg == COLOR_WHITE) && (gui_color[num_color]->attributes & A_BOLD)
            && !CONFIG_BOOLEAN(config_look_color_real_white))
        {
            fg = -1;
        }

        if ((fg > 0) && (fg & GUI_COLOR_EXTENDED_FLAG))
            fg &= GUI_COLOR_EXTENDED_MASK;
        if ((bg > 0) && (bg & GUI_COLOR_EXTENDED_FLAG))
            bg &= GUI_COLOR_EXTENDED_MASK;
        gui_window_set_color (window, fg, bg);
    }
}

/*
 * Sets a custom color for a window (foreground only).
 */

void
gui_window_set_custom_color_fg (WINDOW *window, int fg)
{
    int current_bg, attributes;

    if (fg >= 0)
    {
        current_bg = gui_window_current_style_bg;

        if ((fg > 0) && (fg & GUI_COLOR_EXTENDED_FLAG))
        {
            if (fg & GUI_COLOR_EXTENDED_BLINK_FLAG)
                gui_window_set_color_style (window, A_BLINK);
            else if (!(fg & GUI_COLOR_EXTENDED_KEEPATTR_FLAG))
                gui_window_remove_color_style (window, A_BLINK);
            if (fg & GUI_COLOR_EXTENDED_DIM_FLAG)
                gui_window_set_color_style (window, A_DIM);
            else if (!(fg & GUI_COLOR_EXTENDED_KEEPATTR_FLAG))
                gui_window_remove_color_style (window, A_DIM);
            if (fg & GUI_COLOR_EXTENDED_BOLD_FLAG)
                gui_window_set_color_style (window, A_BOLD);
            else if (!(fg & GUI_COLOR_EXTENDED_KEEPATTR_FLAG))
                gui_window_remove_color_style (window, A_BOLD);
            if (fg & GUI_COLOR_EXTENDED_REVERSE_FLAG)
                gui_window_set_color_style (window, A_REVERSE);
            else if (!(fg & GUI_COLOR_EXTENDED_KEEPATTR_FLAG))
                gui_window_remove_color_style (window, A_REVERSE);
            if (fg & GUI_COLOR_EXTENDED_ITALIC_FLAG)
                gui_window_set_color_style (window, A_ITALIC);
            else if (!(fg & GUI_COLOR_EXTENDED_KEEPATTR_FLAG))
                gui_window_remove_color_style (window, A_ITALIC);
            if (fg & GUI_COLOR_EXTENDED_UNDERLINE_FLAG)
                gui_window_set_color_style (window, A_UNDERLINE);
            else if (!(fg & GUI_COLOR_EXTENDED_KEEPATTR_FLAG))
                gui_window_remove_color_style (window, A_UNDERLINE);
            gui_window_set_color (window,
                                  fg & GUI_COLOR_EXTENDED_MASK,
                                  current_bg);
        }
        else if ((fg & GUI_COLOR_EXTENDED_MASK) < GUI_CURSES_NUM_WEECHAT_COLORS)
        {
            if (!(fg & GUI_COLOR_EXTENDED_KEEPATTR_FLAG))
                gui_window_remove_color_style (window, A_ALL_ATTR);
            attributes = gui_color_get_gui_attrs (fg) |
                gui_weechat_colors[fg & GUI_COLOR_EXTENDED_MASK].attributes;
            gui_window_set_color_style (window, attributes);
            fg = gui_weechat_colors[fg & GUI_COLOR_EXTENDED_MASK].foreground;

            /*
             * if not real white, we use default terminal foreground instead of
             * white if bold attribute is set
             */
            if ((fg == COLOR_WHITE) && (attributes & A_BOLD)
                && !CONFIG_BOOLEAN(config_look_color_real_white))
            {
                fg = -1;
            }

            gui_window_set_color (window, fg, current_bg);
        }
    }
}

/*
 * Sets a custom color for a window (background only).
 */

void
gui_window_set_custom_color_bg (WINDOW *window, int bg)
{
    int current_fg;

    if (bg >= 0)
    {
        current_fg = gui_window_current_style_fg;

        if ((bg > 0) && (bg & GUI_COLOR_EXTENDED_FLAG))
        {
            gui_window_set_color (window,
                                  current_fg,
                                  bg & GUI_COLOR_EXTENDED_MASK);
        }
        else if ((bg & GUI_COLOR_EXTENDED_MASK) < GUI_CURSES_NUM_WEECHAT_COLORS)
        {
            bg &= GUI_COLOR_EXTENDED_MASK;
            gui_window_set_color (window, current_fg,
                                  (gui_color_term_colors >= 16) ?
                                  gui_weechat_colors[bg].background : gui_weechat_colors[bg].foreground);
        }
    }
}

/*
 * Sets a custom color for a window (foreground and background).
 */

void
gui_window_set_custom_color_fg_bg (WINDOW *window, int fg, int bg,
                                   int reset_attributes)
{
    int attributes;

    if ((fg >= 0) && (bg >= 0))
    {
        if ((fg > 0) && (fg & GUI_COLOR_EXTENDED_FLAG))
        {
            if (fg & GUI_COLOR_EXTENDED_BLINK_FLAG)
                gui_window_set_color_style (window, A_BLINK);
            else if (!(fg & GUI_COLOR_EXTENDED_KEEPATTR_FLAG))
                gui_window_remove_color_style (window, A_BLINK);
            if (fg & GUI_COLOR_EXTENDED_DIM_FLAG)
                gui_window_set_color_style (window, A_DIM);
            else if (!(fg & GUI_COLOR_EXTENDED_KEEPATTR_FLAG))
                gui_window_remove_color_style (window, A_DIM);
            if (fg & GUI_COLOR_EXTENDED_BOLD_FLAG)
                gui_window_set_color_style (window, A_BOLD);
            else if (!(fg & GUI_COLOR_EXTENDED_KEEPATTR_FLAG))
                gui_window_remove_color_style (window, A_BOLD);
            if (fg & GUI_COLOR_EXTENDED_REVERSE_FLAG)
                gui_window_set_color_style (window, A_REVERSE);
            else if (!(fg & GUI_COLOR_EXTENDED_KEEPATTR_FLAG))
                gui_window_remove_color_style (window, A_REVERSE);
            if (fg & GUI_COLOR_EXTENDED_ITALIC_FLAG)
                gui_window_set_color_style (window, A_ITALIC);
            else if (!(fg & GUI_COLOR_EXTENDED_KEEPATTR_FLAG))
                gui_window_remove_color_style (window, A_ITALIC);
            if (fg & GUI_COLOR_EXTENDED_UNDERLINE_FLAG)
                gui_window_set_color_style (window, A_UNDERLINE);
            else if (!(fg & GUI_COLOR_EXTENDED_KEEPATTR_FLAG))
                gui_window_remove_color_style (window, A_UNDERLINE);
            fg &= GUI_COLOR_EXTENDED_MASK;
        }
        else if ((fg & GUI_COLOR_EXTENDED_MASK) < GUI_CURSES_NUM_WEECHAT_COLORS)
        {
            if (reset_attributes && !(fg & GUI_COLOR_EXTENDED_KEEPATTR_FLAG))
                gui_window_remove_color_style (window, A_ALL_ATTR);
            attributes = gui_color_get_gui_attrs (fg) |
                gui_weechat_colors[fg & GUI_COLOR_EXTENDED_MASK].attributes;
            gui_window_set_color_style (window, attributes);
            fg = gui_weechat_colors[fg & GUI_COLOR_EXTENDED_MASK].foreground;

            /*
             * if not real white, we use default terminal foreground instead of
             * white if bold attribute is set
             */
            if ((fg == COLOR_WHITE) && (attributes & A_BOLD)
                && !CONFIG_BOOLEAN(config_look_color_real_white))
            {
                fg = -1;
            }
        }

        if ((bg > 0) && (bg & GUI_COLOR_EXTENDED_FLAG))
            bg &= GUI_COLOR_EXTENDED_MASK;
        else
        {
            bg &= GUI_COLOR_EXTENDED_MASK;
            bg = (gui_color_term_colors >= 16) ?
                gui_weechat_colors[bg].background : gui_weechat_colors[bg].foreground;
        }

        gui_window_set_color (window, fg, bg);
    }
}

/*
 * Sets a custom color for a window (pair number).
 */

void
gui_window_set_custom_color_pair (WINDOW *window, int pair)
{
    if ((pair >= 0) && (pair <= gui_color_num_pairs))
    {
        gui_window_remove_color_style (window, A_ALL_ATTR);
        wcolor_set (window, pair, NULL);
    }
}

/*
 * Toggles text emphasis.
 */

void
gui_window_toggle_emphasis ()
{
    gui_window_current_emphasis ^= 1;
}

/*
 * Emphasizes some chars already displayed in a window, either using a color
 * (from config options), or by doing an exclusive or (XOR) with attributes
 * (like reverse video).
 *
 * It is used for example when searching a string in buffer.
 */

void
gui_window_emphasize (WINDOW *window, int x, int y, int count)
{
    attr_t attrs, *ptr_attrs;
    short pair, *ptr_pair;

    if (config_emphasized_attributes == 0)
    {
        /* use color for emphasis (from config) */
        mvwchgat (window, y, x, count,
                  gui_color[GUI_COLOR_EMPHASIS]->attributes,
                  gui_color_weechat_get_pair (GUI_COLOR_EMPHASIS), NULL);
    }
    else
    {
        /* exclusive or (XOR) with attributes */
        attrs = 0;
        pair = 0;
        ptr_attrs = &attrs;
        ptr_pair = &pair;
        wattr_get (window, ptr_attrs, ptr_pair, NULL);
        if (config_emphasized_attributes & GUI_COLOR_EXTENDED_BLINK_FLAG)
            attrs ^= A_BLINK;
        if (config_emphasized_attributes & GUI_COLOR_EXTENDED_DIM_FLAG)
            attrs ^= A_DIM;
        if (config_emphasized_attributes & GUI_COLOR_EXTENDED_BOLD_FLAG)
            attrs ^= A_BOLD;
        if (config_emphasized_attributes & GUI_COLOR_EXTENDED_REVERSE_FLAG)
            attrs ^= A_REVERSE;
        if (config_emphasized_attributes & GUI_COLOR_EXTENDED_ITALIC_FLAG)
            attrs ^= A_ITALIC;
        if (config_emphasized_attributes & GUI_COLOR_EXTENDED_UNDERLINE_FLAG)
            attrs ^= A_UNDERLINE;
        mvwchgat (window, y, x, count, attrs, pair, NULL);
    }

    /* move the cursor after the text (mvwchgat does not move cursor) */
    wmove (window, y, x + count);
}

/*
 * Applies foreground color code in string and moves string pointer after color
 * in string.
 *
 * If window is NULL, no color is applied but string pointer is moved anyway.
 */

void
gui_window_string_apply_color_fg (unsigned char **string, WINDOW *window)
{
    unsigned char *ptr_string;
    char str_fg[6], *error;
    int fg, extra_attr, flag;

    ptr_string = *string;

    if (ptr_string[0] == GUI_COLOR_EXTENDED_CHAR)
    {
        ptr_string++;
        extra_attr = 0;
        while ((flag = gui_color_attr_get_flag (ptr_string[0])) > 0)
        {
            extra_attr |= flag;
            ptr_string++;
        }
        if (ptr_string[0] && ptr_string[1] && ptr_string[2]
            && ptr_string[3] && ptr_string[4])
        {
            if (window)
            {
                memcpy (str_fg, ptr_string, 5);
                str_fg[5] = '\0';
                error = NULL;
                fg = (int)strtol (str_fg, &error, 10);
                if (error && !error[0])
                {
                    gui_window_set_custom_color_fg (window,
                                                    fg | GUI_COLOR_EXTENDED_FLAG | extra_attr);
                }
            }
            ptr_string += 5;
        }
    }
    else
    {
        extra_attr = 0;
        while ((flag = gui_color_attr_get_flag (ptr_string[0])) > 0)
        {
            extra_attr |= flag;
            ptr_string++;
        }
        if (ptr_string[0] && ptr_string[1])
        {
            if (window)
            {
                str_fg[0] = ptr_string[0];
                str_fg[1] = ptr_string[1];
                str_fg[2] = '\0';
                error = NULL;
                fg = (int)strtol (str_fg, &error, 10);
                if (error && !error[0])
                {
                    gui_window_set_custom_color_fg (window, fg | extra_attr);
                }
            }
            ptr_string += 2;
        }
    }

    *string = ptr_string;
}

/*
 * Applies background color code in string and moves string pointer after color
 * in string.
 *
 * If window is NULL, no color is applied but string pointer is moved anyway.
 */

void
gui_window_string_apply_color_bg (unsigned char **string, WINDOW *window)
{
    unsigned char *ptr_string;
    char str_bg[6], *error;
    int bg;

    ptr_string = *string;

    if (ptr_string[0] == GUI_COLOR_EXTENDED_CHAR)
    {
        if (ptr_string[1] && ptr_string[2] && ptr_string[3]
            && ptr_string[4] && ptr_string[5])
        {
            if (window)
            {
                memcpy (str_bg, ptr_string + 1, 5);
                str_bg[5] = '\0';
                error = NULL;
                bg = (int)strtol (str_bg, &error, 10);
                if (error && !error[0])
                {
                    gui_window_set_custom_color_bg (window,
                                                    bg | GUI_COLOR_EXTENDED_FLAG);
                }
            }
            ptr_string += 6;
        }
    }
    else
    {
        if (ptr_string[0] && ptr_string[1])
        {
            if (window)
            {
                str_bg[0] = ptr_string[0];
                str_bg[1] = ptr_string[1];
                str_bg[2] = '\0';
                error = NULL;
                bg = (int)strtol (str_bg, &error, 10);
                if (error && !error[0])
                {
                    gui_window_set_custom_color_bg (window, bg);
                }
            }
            ptr_string += 2;
        }
    }

    *string = ptr_string;
}

/*
 * Applies foreground + background color code in string and moves string pointer
 * after color in string.
 *
 * If window is NULL, no color is applied but string pointer is moved anyway.
 */

void
gui_window_string_apply_color_fg_bg (unsigned char **string, WINDOW *window)
{
    unsigned char *ptr_string;
    char str_fg[6], str_bg[6], *error;
    int fg, bg, extra_attr, flag;

    ptr_string = *string;

    str_fg[0] = '\0';
    str_bg[0] = '\0';
    fg = -1;
    bg = -1;
    if (ptr_string[0] == GUI_COLOR_EXTENDED_CHAR)
    {
        ptr_string++;
        extra_attr = 0;
        while ((flag = gui_color_attr_get_flag (ptr_string[0])) > 0)
        {
            extra_attr |= flag;
            ptr_string++;
        }
        if (ptr_string[0] && ptr_string[1] && ptr_string[2]
            && ptr_string[3] && ptr_string[4])
        {
            if (window)
            {
                memcpy (str_fg, ptr_string, 5);
                str_fg[5] = '\0';
                error = NULL;
                fg = (int)strtol (str_fg, &error, 10);
                if (!error || error[0])
                    fg = -1;
                else
                    fg |= GUI_COLOR_EXTENDED_FLAG | extra_attr;
            }
            ptr_string += 5;
        }
    }
    else
    {
        extra_attr = 0;
        while ((flag = gui_color_attr_get_flag (ptr_string[0])) > 0)
        {
            extra_attr |= flag;
            ptr_string++;
        }
        if (ptr_string[0] && ptr_string[1])
        {
            if (window)
            {
                str_fg[0] = ptr_string[0];
                str_fg[1] = ptr_string[1];
                str_fg[2] = '\0';
                error = NULL;
                fg = (int)strtol (str_fg, &error, 10);
                if (!error || error[0])
                    fg = -1;
                else
                    fg |= extra_attr;
            }
            ptr_string += 2;
        }
    }
    /*
     * note: the comma is an old separator not used any more
     * (since WeeChat 2.6), but we still use it here so in case of /upgrade
     * this will not break colors in old messages
     */
    if ((ptr_string[0] == ',') || (ptr_string[0] == '~'))
    {
        ptr_string++;
        if (ptr_string[0] == GUI_COLOR_EXTENDED_CHAR)
        {
            if (ptr_string[1] && ptr_string[2] && ptr_string[3]
                && ptr_string[4] && ptr_string[5])
            {
                if (window)
                {
                    memcpy (str_bg, ptr_string + 1, 5);
                    str_bg[5] = '\0';
                    error = NULL;
                    bg = (int)strtol (str_bg, &error, 10);
                    if (!error || error[0])
                        bg = -1;
                    else
                        bg |= GUI_COLOR_EXTENDED_FLAG;
                }
                ptr_string += 6;
            }
        }
        else
        {
            if (ptr_string[0] && ptr_string[1])
            {
                if (window)
                {
                    str_bg[0] = ptr_string[0];
                    str_bg[1] = ptr_string[1];
                    str_bg[2] = '\0';
                    error = NULL;
                    bg = (int)strtol (str_bg, &error, 10);
                    if (!error || error[0])
                        bg = -1;
                }
                ptr_string += 2;
            }
        }
    }
    if (window && (fg >= 0) && (bg >= 0))
    {
        gui_window_set_custom_color_fg_bg (window, fg, bg, 1);
    }

    *string = ptr_string;
}

/*
 * Applies pair color code in string and moves string pointer after color in
 * string.
 *
 * If window is NULL, no color is applied but string pointer is moved anyway.
 */

void
gui_window_string_apply_color_pair (unsigned char **string, WINDOW *window)
{
    unsigned char *ptr_string;
    char str_pair[6], *error;
    int pair;

    ptr_string = *string;

    if ((isdigit (ptr_string[0])) && (isdigit (ptr_string[1]))
        && (isdigit (ptr_string[2])) && (isdigit (ptr_string[3]))
        && (isdigit (ptr_string[4])))
    {
        if (window)
        {
            memcpy (str_pair, ptr_string, 5);
            str_pair[5] = '\0';
            error = NULL;
            pair = (int)strtol (str_pair, &error, 10);
            if (error && !error[0])
            {
                gui_window_set_custom_color_pair (window, pair);
            }
        }
        ptr_string += 5;
    }

    *string = ptr_string;
}

/*
 * Applies weechat color code in string and moves string pointer after color in
 * string.
 *
 * If window is NULL, no color is applied but string pointer is moved anyway.
 */

void
gui_window_string_apply_color_weechat (unsigned char **string, WINDOW *window)
{
    unsigned char *ptr_string;
    char str_number[3], *error;
    int weechat_color;

    ptr_string = *string;

    if (isdigit (ptr_string[0]) && isdigit (ptr_string[1]))
    {
        if (window)
        {
            str_number[0] = ptr_string[0];
            str_number[1] = ptr_string[1];
            str_number[2] = '\0';
            error = NULL;
            weechat_color = (int)strtol (str_number, &error, 10);
            if (error && !error[0])
            {
                gui_window_set_weechat_color (window,
                                              weechat_color);
            }
        }
        ptr_string += 2;
    }

    *string = ptr_string;
}

/*
 * Applies "set attribute" color code in string and moves string pointer after
 * color in string.
 *
 * If window is NULL, no color is applied but string pointer is moved anyway.
 */

void
gui_window_string_apply_color_set_attr (unsigned char **string, WINDOW *window)
{
    unsigned char *ptr_string;

    ptr_string = *string;

    switch (ptr_string[0])
    {
        case GUI_COLOR_ATTR_BLINK_CHAR:
            ptr_string++;
            if (window)
                gui_window_set_color_style (window, A_BLINK);
            break;
        case GUI_COLOR_ATTR_DIM_CHAR:
            ptr_string++;
            if (window)
                gui_window_set_color_style (window, A_DIM);
            break;
        case GUI_COLOR_ATTR_BOLD_CHAR:
            ptr_string++;
            if (window)
                gui_window_set_color_style (window, A_BOLD);
            break;
        case GUI_COLOR_ATTR_REVERSE_CHAR:
            ptr_string++;
            if (window)
                gui_window_set_color_style (window, A_REVERSE);
            break;
        case GUI_COLOR_ATTR_ITALIC_CHAR:
            ptr_string++;
            if (window)
                gui_window_set_color_style (window, A_ITALIC);
            break;
        case GUI_COLOR_ATTR_UNDERLINE_CHAR:
            ptr_string++;
            if (window)
                gui_window_set_color_style (window, A_UNDERLINE);
            break;
    }

    *string = ptr_string;
}

/*
 * Applies "remove attribute" color code in string and moves string pointer
 * after color in string.
 *
 * If window is NULL, no color is applied but string pointer is moved anyway.
 */

void
gui_window_string_apply_color_remove_attr (unsigned char **string, WINDOW *window)
{
    unsigned char *ptr_string;

    ptr_string = *string;

    switch (ptr_string[0])
    {
        case GUI_COLOR_ATTR_BLINK_CHAR:
            ptr_string++;
            if (window)
                gui_window_remove_color_style (window, A_BLINK);
            break;
        case GUI_COLOR_ATTR_DIM_CHAR:
            ptr_string++;
            if (window)
                gui_window_remove_color_style (window, A_DIM);
            break;
        case GUI_COLOR_ATTR_BOLD_CHAR:
            ptr_string++;
            if (window)
                gui_window_remove_color_style (window, A_BOLD);
            break;
        case GUI_COLOR_ATTR_REVERSE_CHAR:
            ptr_string++;
            if (window)
                gui_window_remove_color_style (window, A_REVERSE);
            break;
        case GUI_COLOR_ATTR_ITALIC_CHAR:
            ptr_string++;
            if (window)
                gui_window_remove_color_style (window, A_ITALIC);
            break;
        case GUI_COLOR_ATTR_UNDERLINE_CHAR:
            ptr_string++;
            if (window)
                gui_window_remove_color_style (window, A_UNDERLINE);
            break;
    }

    *string = ptr_string;
}

/*
 * Calculates position and size for a buffer and subwindows.
 */

void
gui_window_calculate_pos_size (struct t_gui_window *window)
{
    struct t_gui_bar_window *ptr_bar_win;
    int add_top, add_bottom, add_left, add_right;

    for (ptr_bar_win = window->bar_windows; ptr_bar_win;
         ptr_bar_win = ptr_bar_win->next_bar_window)
    {
        gui_bar_window_calculate_pos_size (ptr_bar_win, window);
    }

    add_bottom = gui_bar_window_get_size (NULL, window, GUI_BAR_POSITION_BOTTOM);
    add_top = gui_bar_window_get_size (NULL, window, GUI_BAR_POSITION_TOP);
    add_left = gui_bar_window_get_size (NULL, window, GUI_BAR_POSITION_LEFT);
    add_right = gui_bar_window_get_size (NULL, window, GUI_BAR_POSITION_RIGHT);

    window->win_chat_x = window->win_x + add_left;
    window->win_chat_y = window->win_y + add_top;
    window->win_chat_width = window->win_width - add_left - add_right;
    window->win_chat_height = window->win_height - add_top - add_bottom;
    window->win_chat_cursor_x = window->win_x + add_left;
    window->win_chat_cursor_y = window->win_y + add_top;

    /* chat area too small? (not enough space left) */
    if ((window->win_chat_width < 1) || (window->win_chat_height < 1))
    {
        /* invalidate the chat area, it will not be displayed */
        window->win_chat_x = -1;
        window->win_chat_y = -1;
        window->win_chat_width = 0;
        window->win_chat_height = 0;
        window->win_chat_cursor_x = 0;
        window->win_chat_cursor_y = 0;
    }
}

/*
 * Draws a horizontal line (like ncurses function "mvwhline", but UTF-8 chars
 * are supported).
 *
 * If "string" is NULL or empty, the ACS_HLINE char is used (plain line).
 * If "string" is not NULL and not empty, its width on screen must be exactly
 * one char.
 */

void
gui_window_hline (WINDOW *window, int x, int y, int width, const char *string)
{
    int i;

    if (string && string[0])
    {
        for (i = 0; i < width; i++)
        {
            mvwaddstr (window, y, x + i, string);
        }
    }
    else
    {
        mvwhline (window, y, x, ACS_HLINE, width);
    }
}

/*
 * Draws a vertical line (like ncurses function "mvwvline", but UTF-8 chars
 * are supported).
 *
 * If "string" is NULL or empty, the ACS_VLINE char is used (plain line).
 * If "string" is not NULL and not empty, its width on screen must be exactly
 * one char.
 */

void
gui_window_vline (WINDOW *window, int x, int y, int height, const char *string)
{
    int i;

    if (string && string[0])
    {
        for (i = 0; i < height; i++)
        {
            mvwaddstr (window, y + i, x, string);
        }
    }
    else
    {
        mvwvline (window, y, x, ACS_VLINE, height);
    }
}

/*
 * Draws window separators.
 */

void
gui_window_draw_separators (struct t_gui_window *window)
{
    int separator_horizontal, separator_vertical;
    int horiz_overlap, x, width, height;
    struct t_gui_window_tree *ptr_tree, *ptr_child;

    /* remove separators */
    if (GUI_WINDOW_OBJECTS(window)->win_separator_horiz)
    {
        delwin (GUI_WINDOW_OBJECTS(window)->win_separator_horiz);
        GUI_WINDOW_OBJECTS(window)->win_separator_horiz = NULL;
    }
    if (GUI_WINDOW_OBJECTS(window)->win_separator_vertic)
    {
        delwin (GUI_WINDOW_OBJECTS(window)->win_separator_vertic);
        GUI_WINDOW_OBJECTS(window)->win_separator_vertic = NULL;
    }

    /* check if separators must be displayed */
    separator_horizontal = (CONFIG_BOOLEAN(config_look_window_separator_horizontal)
                            && (window->win_y + window->win_height <
                                gui_window_get_height () - gui_bar_root_get_size (NULL, GUI_BAR_POSITION_BOTTOM) - 1));
    separator_vertical = (CONFIG_BOOLEAN(config_look_window_separator_vertical)
                          && (window->win_x > gui_bar_root_get_size (NULL, GUI_BAR_POSITION_LEFT)));

    /* check if horizontal separator must overlap in lower left corner */
    horiz_overlap = 0;
    ptr_child = window->ptr_tree;
    ptr_tree = ptr_child->parent_node;
    while (ptr_tree)
    {
        /* is right/top child */
        if (ptr_tree->child2 == ptr_child)
        {
            /*
             * horizontal overlap is needed when window is right in vertical
             * split; it is not needed when window is top in horizontal split
             */
            horiz_overlap = !ptr_tree->split_horizontal;
            break;
        }

        ptr_child = ptr_child->parent_node;
        ptr_tree = ptr_tree->parent_node;
    }

    /* create/draw horizontal separator */
    if (separator_horizontal)
    {
        x = (separator_vertical && horiz_overlap) ?
            window->win_x - 1 : window->win_x;
        width = (separator_vertical && horiz_overlap) ?
            window->win_width + 1 : window->win_width;
        GUI_WINDOW_OBJECTS(window)->win_separator_horiz = newwin (
            1,
            width,
            window->win_y + window->win_height,
            x);
        gui_window_set_weechat_color (
            GUI_WINDOW_OBJECTS(window)->win_separator_horiz,
            GUI_COLOR_SEPARATOR);
        gui_window_hline (GUI_WINDOW_OBJECTS(window)->win_separator_horiz,
                          0, 0, width,
                          CONFIG_STRING(config_look_separator_horizontal));
        wnoutrefresh (GUI_WINDOW_OBJECTS(window)->win_separator_horiz);
    }

    /* create/draw vertical separator */
    if (separator_vertical)
    {
        height = (separator_horizontal && !horiz_overlap) ?
            window->win_height + 1 : window->win_height;
        GUI_WINDOW_OBJECTS(window)->win_separator_vertic = newwin (
            height,
            1,
            window->win_y,
            window->win_x - 1);
        gui_window_set_weechat_color (
            GUI_WINDOW_OBJECTS(window)->win_separator_vertic,
            GUI_COLOR_SEPARATOR);
        gui_window_vline (GUI_WINDOW_OBJECTS(window)->win_separator_vertic,
                          0, 0, height,
                          CONFIG_STRING(config_look_separator_vertical));
        wnoutrefresh (GUI_WINDOW_OBJECTS(window)->win_separator_vertic);
    }
}

/*
 * Switches to another buffer in a window.
 */

void
gui_window_switch_to_buffer (struct t_gui_window *window,
                             struct t_gui_buffer *buffer,
                             int set_last_read)
{
    struct t_gui_bar_window *ptr_bar_window;
    struct t_gui_buffer *old_buffer;

    if (!gui_init_ok)
        return;

    gui_buffer_add_value_num_displayed (window->buffer, -1);

    old_buffer = window->buffer;

    if (window->buffer->number != buffer->number)
    {
        gui_buffer_last_displayed = window->buffer;
        gui_window_scroll_switch (window, buffer);
        if ((buffer->type == GUI_BUFFER_TYPE_FORMATTED)
            && CONFIG_BOOLEAN(config_look_scroll_bottom_after_switch))
        {
            window->scroll->start_line = NULL;
            window->scroll->start_line_pos = 0;
            window->scroll->scrolling = 0;
        }
        if (!gui_buffers_visited_frozen)
        {
            gui_buffer_visited_add (window->buffer);
            gui_buffer_visited_add (buffer);
        }
        if (set_last_read
            && CONFIG_BOOLEAN(config_look_read_marker_update_on_buffer_switch))
        {
            if (window->buffer->num_displayed == 0)
            {
                window->buffer->lines->last_read_line = window->buffer->lines->last_line;
                window->buffer->lines->first_line_not_read = 0;
            }
        }
        if (set_last_read)
        {
            /*
             * if there is no line displayed after last read line,
             * then remove the read marker
             */
            if (buffer->lines->last_read_line
                && !gui_line_get_next_displayed (buffer->lines->last_read_line))
            {
                buffer->lines->last_read_line = NULL;
                buffer->lines->first_line_not_read = 0;
            }
        }
    }

    window->buffer = buffer;

    gui_buffer_set_active_buffer (buffer);
    gui_buffer_compute_num_displayed ();

    if (!weechat_upgrading
        && (old_buffer != buffer)
        && CONFIG_BOOLEAN(config_look_hotlist_update_on_buffer_switch))
    {
        gui_hotlist_remove_buffer (buffer, 0);
    }

    /* remove unused bars and add missing bars in window */
    gui_bar_window_remove_unused_bars (window);
    gui_bar_window_add_missing_bars (window);

    /* create bar windows */
    for (ptr_bar_window = window->bar_windows; ptr_bar_window;
         ptr_bar_window = ptr_bar_window->next_bar_window)
    {
        gui_bar_window_content_build (ptr_bar_window, window);
        gui_bar_window_calculate_pos_size (ptr_bar_window, window);
        gui_bar_window_create_win (ptr_bar_window);
    }

    gui_window_calculate_pos_size (window);

    /* destroy Curses windows */
    gui_window_objects_free (window, 0);

    /* create Curses windows */
    if (GUI_WINDOW_OBJECTS(window)->win_chat)
    {
        delwin (GUI_WINDOW_OBJECTS(window)->win_chat);
        GUI_WINDOW_OBJECTS(window)->win_chat = NULL;
    }
    if ((window->win_chat_x >= 0) && (window->win_chat_y >= 0))
    {
        GUI_WINDOW_OBJECTS(window)->win_chat = newwin (window->win_chat_height,
                                                       window->win_chat_width,
                                                       window->win_chat_y,
                                                       window->win_chat_x);
    }
    gui_window_draw_separators (window);
    gui_buffer_ask_chat_refresh (window->buffer, 2);

    if (window->buffer->type == GUI_BUFFER_TYPE_FREE)
    {
        window->scroll->scrolling = 0;
        window->scroll->lines_after = 0;
    }

    for (ptr_bar_window = window->bar_windows; ptr_bar_window;
         ptr_bar_window = ptr_bar_window->next_bar_window)
    {
        ptr_bar_window->bar->bar_refresh_needed = 1;
    }

    if (CONFIG_BOOLEAN(config_look_read_marker_always_show)
        && set_last_read
        && !window->buffer->lines->last_read_line
        && !window->buffer->lines->first_line_not_read)
    {
        window->buffer->lines->last_read_line = window->buffer->lines->last_line;
    }

    gui_buffer_input_move_to_buffer (old_buffer, window->buffer);

    if (old_buffer != buffer)
    {
        (void) hook_signal_send ("buffer_switch",
                                 WEECHAT_HOOK_SIGNAL_POINTER, buffer);
    }
}

/*
 * Switches to another window.
 */

void
gui_window_switch (struct t_gui_window *window)
{
    struct t_gui_window *old_window;
    int changes;

    if (gui_current_window == window)
        return;

    old_window = gui_current_window;

    gui_current_window = window;

    /* remove unused bars and add missing bars in window */
    changes = gui_bar_window_remove_unused_bars (old_window)
        || gui_bar_window_add_missing_bars (old_window);
    if (changes)
    {
        gui_current_window = old_window;
        gui_window_switch_to_buffer (gui_current_window,
                                     gui_current_window->buffer, 1);
        gui_current_window = window;
    }

    gui_window_switch_to_buffer (gui_current_window,
                                 gui_current_window->buffer, 1);

    old_window->refresh_needed = 1;

    gui_buffer_input_move_to_buffer (old_window->buffer, window->buffer);

    (void) hook_signal_send ("window_switch",
                             WEECHAT_HOOK_SIGNAL_POINTER, gui_current_window);
}

/*
 * Displays previous page on buffer.
 */

void
gui_window_page_up (struct t_gui_window *window)
{
    char scroll[32];
    int height, num_lines;

    if (!gui_init_ok)
        return;

    height = (gui_window_bare_display) ?
        gui_term_lines : window->win_chat_height;

    num_lines = ((height - 1) *
                 CONFIG_INTEGER(config_look_scroll_page_percent)) / 100;
    if (num_lines < 1)
        num_lines = 1;
    else if (num_lines > height - 1)
        num_lines = height - 1;

    switch (window->buffer->type)
    {
        case GUI_BUFFER_TYPE_FORMATTED:
            if (!window->scroll->first_line_displayed)
            {
                gui_chat_calculate_line_diff (window, &window->scroll->start_line,
                                              &window->scroll->start_line_pos,
                                              (window->scroll->start_line) ?
                                              (-1) * (num_lines) :
                                              (-1) * (num_lines + height - 1));
                gui_buffer_ask_chat_refresh (window->buffer, 2);
            }
            break;
        case GUI_BUFFER_TYPE_FREE:
            if (window->scroll->start_line)
            {
                snprintf (scroll, sizeof (scroll), "-%d",
                          num_lines + 1);
                gui_window_scroll (window, scroll);
                (void) hook_signal_send ("window_scrolled",
                                         WEECHAT_HOOK_SIGNAL_POINTER, window);
            }
            break;
        case GUI_BUFFER_NUM_TYPES:
            break;
    }
}

/*
 * Displays next page on buffer.
 */

void
gui_window_page_down (struct t_gui_window *window)
{
    struct t_gui_line *ptr_line;
    int height, num_lines, line_pos;
    char scroll[32];

    if (!gui_init_ok)
        return;

    height = (gui_window_bare_display) ?
        gui_term_lines : window->win_chat_height;

    num_lines = ((height - 1) *
                 CONFIG_INTEGER(config_look_scroll_page_percent)) / 100;
    if (num_lines < 1)
        num_lines = 1;
    else if (num_lines > height - 1)
        num_lines = height - 1;

    switch (window->buffer->type)
    {
        case GUI_BUFFER_TYPE_FORMATTED:
            if (window->scroll->start_line
                && (window->scroll->start_line_pos >= 0))
            {
                gui_chat_calculate_line_diff (window, &window->scroll->start_line,
                                              &window->scroll->start_line_pos,
                                              num_lines);

                /* check if we can display all lines in chat area */
                ptr_line = window->scroll->start_line;
                line_pos = window->scroll->start_line_pos;
                gui_chat_calculate_line_diff (window, &ptr_line, &line_pos,
                                              height);
                if (!ptr_line)
                {
                    window->scroll->start_line = NULL;
                    window->scroll->start_line_pos = 0;
                }
                gui_buffer_ask_chat_refresh (window->buffer, 2);
            }
            break;
        case GUI_BUFFER_TYPE_FREE:
            snprintf (scroll, sizeof (scroll), "+%d",
                      num_lines + 1);
            gui_window_scroll (window, scroll);
            (void) hook_signal_send ("window_scrolled",
                                     WEECHAT_HOOK_SIGNAL_POINTER, window);
            break;
        case GUI_BUFFER_NUM_TYPES:
            break;
    }
}

/*
 * Displays previous few lines in buffer.
 */

void
gui_window_scroll_up (struct t_gui_window *window)
{
    int height;
    char scroll[32];

    if (!gui_init_ok)
        return;

    height = (gui_window_bare_display) ?
        gui_term_lines : window->win_chat_height;

    switch (window->buffer->type)
    {
        case GUI_BUFFER_TYPE_FORMATTED:
            if (!window->scroll->first_line_displayed)
            {
                gui_chat_calculate_line_diff (window, &window->scroll->start_line,
                                              &window->scroll->start_line_pos,
                                              (window->scroll->start_line) ?
                                              (-1) * CONFIG_INTEGER(config_look_scroll_amount) :
                                              (-1) * ((height - 1) +
                                                      CONFIG_INTEGER(config_look_scroll_amount)));
                gui_buffer_ask_chat_refresh (window->buffer, 2);
            }
            break;
        case GUI_BUFFER_TYPE_FREE:
            if (window->scroll->start_line)
            {
                snprintf (scroll, sizeof (scroll), "-%d",
                          CONFIG_INTEGER(config_look_scroll_amount));
                gui_window_scroll (window, scroll);
                (void) hook_signal_send ("window_scrolled",
                                         WEECHAT_HOOK_SIGNAL_POINTER, window);
            }
            break;
        case GUI_BUFFER_NUM_TYPES:
            break;
    }
}

/*
 * Displays next few lines in buffer.
 */

void
gui_window_scroll_down (struct t_gui_window *window)
{
    struct t_gui_line *ptr_line;
    int height, line_pos;
    char scroll[32];

    if (!gui_init_ok)
        return;

    height = (gui_window_bare_display) ?
        gui_term_lines : window->win_chat_height;

    switch (window->buffer->type)
    {
        case GUI_BUFFER_TYPE_FORMATTED:
            if (window->scroll->start_line
                && (window->scroll->start_line_pos >= 0))
            {
                gui_chat_calculate_line_diff (window, &window->scroll->start_line,
                                              &window->scroll->start_line_pos,
                                              CONFIG_INTEGER(config_look_scroll_amount));

                /* check if we can display all lines in chat area */
                ptr_line = window->scroll->start_line;
                line_pos = window->scroll->start_line_pos;
                gui_chat_calculate_line_diff (window, &ptr_line, &line_pos,
                                              height);

                if (!ptr_line)
                {
                    window->scroll->start_line = NULL;
                    window->scroll->start_line_pos = 0;
                }
                gui_buffer_ask_chat_refresh (window->buffer, 2);
            }
            break;
        case GUI_BUFFER_TYPE_FREE:
            snprintf (scroll, sizeof (scroll), "+%d",
                      CONFIG_INTEGER(config_look_scroll_amount));
            gui_window_scroll (window, scroll);
            (void) hook_signal_send ("window_scrolled",
                                     WEECHAT_HOOK_SIGNAL_POINTER, window);
            break;
        case GUI_BUFFER_NUM_TYPES:
            break;
    }
}

/*
 * Scrolls to top of buffer.
 */

void
gui_window_scroll_top (struct t_gui_window *window)
{
    if (!gui_init_ok)
        return;

    switch (window->buffer->type)
    {
        case GUI_BUFFER_TYPE_FORMATTED:
            if (!window->scroll->first_line_displayed)
            {
                window->scroll->start_line = gui_line_get_first_displayed (window->buffer);
                window->scroll->start_line_pos = 0;
                gui_buffer_ask_chat_refresh (window->buffer, 2);
            }
            break;
        case GUI_BUFFER_TYPE_FREE:
            if (window->scroll->start_line)
            {
                window->scroll->start_line = NULL;
                gui_buffer_ask_chat_refresh (window->buffer, 2);
                (void) hook_signal_send ("window_scrolled",
                                         WEECHAT_HOOK_SIGNAL_POINTER, window);
            }
            break;
        case GUI_BUFFER_NUM_TYPES:
            break;
    }
}

/*
 * Scrolls to bottom of buffer.
 */

void
gui_window_scroll_bottom (struct t_gui_window *window)
{
    int height;
    char scroll[32];

    if (!gui_init_ok)
        return;

    height = (gui_window_bare_display) ?
        gui_term_lines : window->win_chat_height;

    switch (window->buffer->type)
    {
        case GUI_BUFFER_TYPE_FORMATTED:
            window->scroll->start_line = NULL;
            window->scroll->start_line_pos = 0;
            gui_buffer_ask_chat_refresh (window->buffer, 2);
            break;
        case GUI_BUFFER_TYPE_FREE:
            window->scroll->start_line = NULL;
            if (window->buffer->lines->lines_count > height)
            {
                snprintf (scroll, sizeof (scroll), "--%d", height - 1);
                gui_window_scroll (window, scroll);
            }
            else
            {
                gui_buffer_ask_chat_refresh (window->buffer, 2);
            }
            (void) hook_signal_send ("window_scrolled",
                                     WEECHAT_HOOK_SIGNAL_POINTER, window);
            break;
        case GUI_BUFFER_NUM_TYPES:
            break;
    }
}

/*
 * Scrolls beyond the end of buffer (so that all lines become "hidden" above the
 * top of window).
 */

void
gui_window_scroll_beyond_end (struct t_gui_window *window)
{
    if (!gui_init_ok)
        return;

    if (window->buffer->lines->last_line)
    {
        window->scroll->start_line = window->buffer->lines->last_line;
        window->scroll->start_line_pos = -1;
        window->scroll->first_line_displayed = 0;
        gui_buffer_ask_chat_refresh (window->buffer, 2);
    }
}

/*
 * Auto-resizes all windows, according to % of global size.
 *
 * This function is called after a terminal resize.
 *
 * Returns:
 *    0: OK
 *   -1: all windows must be merged (not enough space)
 */

int
gui_window_auto_resize (struct t_gui_window_tree *tree,
                        int x, int y, int width, int height,
                        int simulate)
{
    int size1, size2, separator;
    struct t_gui_window_tree *parent;

    if (!gui_init_ok)
        return 0;

    if (tree)
    {
        if (tree->window)
        {
            if ((width < 1) || (height < 2))
                return -1;
            if (!simulate)
            {
                tree->window->win_x = x;
                tree->window->win_y = y;
                tree->window->win_width = width;
                tree->window->win_height = height;
                parent = tree->parent_node;
                if (parent)
                {
                    if (parent->split_horizontal)
                    {
                        tree->window->win_width_pct = 100;
                        tree->window->win_height_pct = (tree == parent->child1) ?
                            100 - parent->split_pct : parent->split_pct;
                    }
                    else
                    {
                        tree->window->win_width_pct = (tree == parent->child1) ?
                            100 - parent->split_pct : parent->split_pct;
                        tree->window->win_height_pct = 100;
                    }
                }
            }
        }
        else
        {
            if (tree->split_horizontal)
            {
                separator = (CONFIG_BOOLEAN(config_look_window_separator_horizontal)) ? 1 : 0;
                size1 = ((height - separator) * tree->split_pct) / 100;
                size2 = height - size1 - separator;
                if (gui_window_auto_resize (tree->child1, x, y + size1 + separator,
                                            width, size2, simulate) < 0)
                    return -1;
                if (gui_window_auto_resize (tree->child2, x, y,
                                            width, size1, simulate) < 0)
                    return -1;
            }
            else
            {
                separator = (CONFIG_BOOLEAN(config_look_window_separator_vertical)) ? 1 : 0;
                size2 = (width * tree->split_pct) / 100;
                size1 = width - size2 - separator;
                if (gui_window_auto_resize (tree->child1, x, y,
                                            size1, height, simulate) < 0)
                    return -1;
                if (gui_window_auto_resize (tree->child2, x + size1 + separator, y,
                                            size2, height, simulate) < 0)
                    return -1;
            }
        }
    }
    return 0;
}

/*
 * Auto-resizes and refreshes all windows.
 */

void
gui_window_refresh_windows ()
{
    struct t_gui_window *ptr_win, *old_current_window;
    struct t_gui_bar_window *ptr_bar_win;
    struct t_gui_bar *ptr_bar;
    struct t_gui_layout *ptr_layout;
    int add_bottom, add_top, add_left, add_right;

    if (!gui_init_ok)
        return;

    old_current_window = gui_current_window;

    /* remove unused bars and add missing root bars */
    gui_bar_window_remove_unused_bars (NULL);
    gui_bar_window_add_missing_bars (NULL);

    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if ((CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
            && ptr_bar->bar_window
            && !CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
        {
            gui_bar_window_calculate_pos_size (ptr_bar->bar_window, NULL);
            gui_bar_window_create_win (ptr_bar->bar_window);
            gui_bar_ask_refresh (ptr_bar);
        }
    }

    add_bottom = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_BOTTOM);
    add_top = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_TOP);
    add_left = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_LEFT);
    add_right = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_RIGHT);

    if (gui_window_auto_resize (gui_windows_tree, add_left, add_top,
                                gui_window_get_width () - add_left - add_right,
                                gui_window_get_height () - add_top - add_bottom,
                                0) < 0)
    {
        if (CONFIG_BOOLEAN(config_look_window_auto_zoom))
        {
            ptr_layout = gui_layout_search (GUI_LAYOUT_ZOOM);
            if (ptr_layout)
            {
                /* remove zoom saved, to force a new zoom */
                gui_layout_remove (ptr_layout);
            }
            gui_window_zoom (gui_current_window);
        }
    }

    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        gui_window_calculate_pos_size (ptr_win);
        for (ptr_bar_win = ptr_win->bar_windows; ptr_bar_win;
             ptr_bar_win = ptr_bar_win->next_bar_window)
        {
            gui_bar_window_create_win (ptr_bar_win);
        }
        ptr_win->refresh_needed = 1;
    }

    gui_current_window = old_current_window;
}

/*
 * Horizontally splits a window.
 *
 * Returns pointer to new window, NULL if error.
 */

struct t_gui_window *
gui_window_split_horizontal (struct t_gui_window *window, int percentage)
{
    struct t_gui_window *new_window;
    int height1, height2, separator;

    if (!gui_init_ok)
        return NULL;

    new_window = NULL;

    separator = (CONFIG_BOOLEAN(config_look_window_separator_horizontal)) ? 1 : 0;

    height1 = ((window->win_height - separator) * percentage) / 100;
    height2 = window->win_height - height1 - separator;

    if ((height1 >= 2) && (height2 >= 2)
        && (percentage > 0) && (percentage < 100))
    {
        new_window = gui_window_new (window, window->buffer,
                                     window->win_x,
                                     window->win_y,
                                     window->win_width,
                                     height1,
                                     100, percentage);
        if (new_window)
        {
            /* reduce old window height (bottom window) */
            window->win_y = new_window->win_y + new_window->win_height;
            window->win_height = height2;
            window->win_height_pct = 100 - percentage;

            /* assign same buffer for new window (top window) */
            gui_buffer_add_value_num_displayed (new_window->buffer, 1);

            gui_window_ask_refresh (1);

            gui_window_switch (new_window);

            /* create & draw separators */
            gui_window_draw_separators (gui_current_window);
        }
    }

    return new_window;
}

/*
 * Vertically splits a window.
 *
 * Returns pointer to new window, NULL if error.
 */

struct t_gui_window *
gui_window_split_vertical (struct t_gui_window *window, int percentage)
{
    struct t_gui_window *new_window;
    int width1, width2, separator;

    if (!gui_init_ok)
        return NULL;

    new_window = NULL;

    separator = (CONFIG_BOOLEAN(config_look_window_separator_vertical)) ? 1 : 0;

    width2 = (window->win_width * percentage) / 100;
    width1 = window->win_width - width2 - separator;

    if ((width1 >= 1) && (width2 >= 1)
        && (percentage > 0) && (percentage < 100))
    {
        new_window = gui_window_new (window, window->buffer,
                                     window->win_x + width1 + separator,
                                     window->win_y,
                                     width2,
                                     window->win_height,
                                     percentage, 100);
        if (new_window)
        {
            /* reduce old window height (left window) */
            window->win_width = width1;
            window->win_width_pct = 100 - percentage;

            /* assign same buffer for new window (right window) */
            gui_buffer_add_value_num_displayed (new_window->buffer, 1);

            gui_window_ask_refresh (1);

            gui_window_switch (new_window);

            /* create & draw separators */
            gui_window_draw_separators (gui_current_window);
        }
    }

    return new_window;
}

/*
 * Resizes a window.
 */

void
gui_window_resize (struct t_gui_window_tree *tree, int percentage)
{
    struct t_gui_window_tree *parent;
    int old_split_pct, add_bottom, add_top, add_left, add_right;

    if (!gui_init_ok)
        return;

    parent = tree->parent_node;
    if (parent)
    {
        old_split_pct = parent->split_pct;
        if (tree == parent->child2)
            parent->split_pct = percentage;
        else
            parent->split_pct = 100 - percentage;

        add_bottom = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_BOTTOM);
        add_top = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_TOP);
        add_left = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_LEFT);
        add_right = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_RIGHT);

        if (gui_window_auto_resize (gui_windows_tree, add_left, add_top,
                                    gui_window_get_width () - add_left - add_right,
                                    gui_window_get_height () - add_top - add_bottom,
                                    1) < 0)
            parent->split_pct = old_split_pct;
        else
            gui_window_ask_refresh (1);
    }
}

/*
 * Resizes window using delta percentage.
 */

void
gui_window_resize_delta (struct t_gui_window_tree *tree, int delta_percentage)
{
    struct t_gui_window_tree *parent;
    int old_split_pct, add_bottom, add_top, add_left, add_right;

    if (!gui_init_ok)
        return;

    parent = tree->parent_node;
    if (parent)
    {
        old_split_pct = parent->split_pct;
        if (tree == parent->child2)
            parent->split_pct += delta_percentage;
        else
            parent->split_pct -= delta_percentage;
        if (parent->split_pct < 1)
            parent->split_pct = 1;
        else if (parent->split_pct > 99)
            parent->split_pct = 99;

        add_bottom = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_BOTTOM);
        add_top = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_TOP);
        add_left = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_LEFT);
        add_right = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_RIGHT);

        if (gui_window_auto_resize (gui_windows_tree, add_left, add_top,
                                    gui_window_get_width () - add_left - add_right,
                                    gui_window_get_height () - add_top - add_bottom,
                                    1) < 0)
            parent->split_pct = old_split_pct;
        else
            gui_window_ask_refresh (1);
    }
}

/*
 * Merges window with its sister.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
gui_window_merge (struct t_gui_window *window)
{
    struct t_gui_window_tree *parent, *sister;
    int separator;

    if (!gui_init_ok)
        return 0;

    parent = window->ptr_tree->parent_node;
    if (!parent)
        return 0;

    sister = (parent->child1->window == window) ?
        parent->child2 : parent->child1;
    if (!(sister->window))
        return 0;

    if (window->win_y == sister->window->win_y)
    {
        /* horizontal merge */
        separator = (CONFIG_BOOLEAN(config_look_window_separator_horizontal)) ? 1 : 0;
        window->win_width += sister->window->win_width + separator;
        window->win_width_pct += sister->window->win_width_pct;
    }
    else
    {
        /* vertical merge */
        separator = (CONFIG_BOOLEAN(config_look_window_separator_vertical)) ? 1 : 0;
        window->win_height += sister->window->win_height + separator;
        window->win_height_pct += sister->window->win_height_pct;
    }
    if (sister->window->win_x < window->win_x)
        window->win_x = sister->window->win_x;
    if (sister->window->win_y < window->win_y)
        window->win_y = sister->window->win_y;

    gui_window_free (sister->window);
    gui_window_tree_node_to_leaf (parent, window);

    gui_window_switch_to_buffer (window, window->buffer, 1);

    return 1;
}

/*
 * Merges all windows into only one.
 */

void
gui_window_merge_all (struct t_gui_window *window)
{
    int num_deleted, add_bottom, add_top, add_left, add_right;

    if (!gui_init_ok)
        return;

    num_deleted = 0;
    while (gui_windows->next_window)
    {
        gui_window_free ((gui_windows == window) ? gui_windows->next_window : gui_windows);
        num_deleted++;
    }

    if (num_deleted > 0)
    {
        gui_window_tree_free (&gui_windows_tree);
        gui_window_tree_init (window);
        window->ptr_tree = gui_windows_tree;

        add_bottom = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_BOTTOM);
        add_top = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_TOP);
        add_left = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_LEFT);
        add_right = gui_bar_root_get_size (NULL, GUI_BAR_POSITION_RIGHT);
        window->win_x = add_left;
        window->win_y = add_top;
        window->win_width = gui_window_get_width () - add_left - add_right;
        window->win_height = gui_window_get_height () - add_top - add_bottom;

        window->win_width_pct = 100;
        window->win_height_pct = 100;

        gui_current_window = window;
        gui_window_switch_to_buffer (window, window->buffer, 1);
    }
}

/*
 * Closes a window.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
gui_window_close (struct t_gui_window *window)
{
    struct t_gui_window_tree *parent, *sister;

    if (!gui_init_ok)
        return 0;

    parent = window->ptr_tree->parent_node;
    if (!parent)
        return 0;

    sister = (parent->child1->window == window) ?
        parent->child2 : parent->child1;
    if (!(sister->window))
        return 0;

    return gui_window_merge (sister->window);
}

/*
 * Returns a code about position of 2 windows:
 *   0 = they're not side by side
 *   1 = side by side: win2 is over the win1
 *   2 = side by side: win2 on the right
 *   3 = side by side: win2 below win1
 *   4 = side by side: win2 on the left
 */

int
gui_window_side_by_side (struct t_gui_window *win1, struct t_gui_window *win2)
{
    int separator_horizontal, separator_vertical;

    if (!gui_init_ok)
        return 0;

    separator_horizontal = (CONFIG_BOOLEAN(config_look_window_separator_horizontal)) ? 1 : 0;
    separator_vertical = (CONFIG_BOOLEAN(config_look_window_separator_vertical)) ? 1 : 0;

    /* win2 over win1 ? */
    if (win2->win_y + win2->win_height + separator_horizontal == win1->win_y)
    {
        if (win2->win_x >= win1->win_x + win1->win_width)
            return 0;
        if (win2->win_x + win2->win_width <= win1->win_x)
            return 0;
        return 1;
    }

    /* win2 on the right ? */
    if (win2->win_x == win1->win_x + win1->win_width + separator_vertical)
    {
        if (win2->win_y >= win1->win_y + win1->win_height)
            return 0;
        if (win2->win_y + win2->win_height <= win1->win_y)
            return 0;
        return 2;
    }

    /* win2 below win1 ? */
    if (win2->win_y == win1->win_y + win1->win_height + separator_horizontal)
    {
        if (win2->win_x >= win1->win_x + win1->win_width)
            return 0;
        if (win2->win_x + win2->win_width <= win1->win_x)
            return 0;
        return 3;
    }

    /* win2 on the left ? */
    if (win2->win_x + win2->win_width + separator_vertical == win1->win_x)
    {
        if (win2->win_y >= win1->win_y + win1->win_height)
            return 0;
        if (win2->win_y + win2->win_height <= win1->win_y)
            return 0;
        return 4;
    }

    return 0;
}

/*
 * Searches and switches to a window over current window.
 */

void
gui_window_switch_up (struct t_gui_window *window)
{
    struct t_gui_window *ptr_win;

    if (!gui_init_ok)
        return;

    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if ((ptr_win != window) &&
            (gui_window_side_by_side (window, ptr_win) == 1))
        {
            gui_window_switch (ptr_win);
            return;
        }
    }
}

/*
 * Searches and switches to a window below current window.
 */

void
gui_window_switch_down (struct t_gui_window *window)
{
    struct t_gui_window *ptr_win;

    if (!gui_init_ok)
        return;

    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if ((ptr_win != window) &&
            (gui_window_side_by_side (window, ptr_win) == 3))
        {
            gui_window_switch (ptr_win);
            return;
        }
    }
}

/*
 * Searches and switches to a window on the left of current window.
 */

void
gui_window_switch_left (struct t_gui_window *window)
{
    struct t_gui_window *ptr_win;

    if (!gui_init_ok)
        return;

    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if ((ptr_win != window) &&
            (gui_window_side_by_side (window, ptr_win) == 4))
        {
            gui_window_switch (ptr_win);
            return;
        }
    }
}

/*
 * Searches and switches to a window on the right of current window.
 */

void
gui_window_switch_right (struct t_gui_window *window)
{
    struct t_gui_window *ptr_win;

    if (!gui_init_ok)
        return;

    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if ((ptr_win != window) &&
            (gui_window_side_by_side (window, ptr_win) == 2))
        {
            gui_window_switch (ptr_win);
            return;
        }
    }
}

/*
 * Counts number of windows in a tree with a given split, for balancing windows.
 */

int
gui_window_balance_count (struct t_gui_window_tree *tree, int split_horizontal)
{
    int count;

    count = 0;
    if (tree)
    {
        if (!tree->window && (tree->split_horizontal == split_horizontal))
        {
            if ((tree->child1 && tree->child1->window)
                || (tree->child2 && tree->child2->window))
            {
                count++;
            }
        }
        count += gui_window_balance_count (tree->child1, split_horizontal);
        count += gui_window_balance_count (tree->child2, split_horizontal);
    }
    return count;
}

/*
 * Balances windows (set all splits to 50%).
 *
 * Returns:
 *   1: some windows have been balanced
 *   0: nothing was changed
 */

int
gui_window_balance (struct t_gui_window_tree *tree)
{
    int balanced, count_left, count_right, new_split_pct;

    balanced = 0;
    if (tree && tree->child1 && tree->child2)
    {
        count_left = gui_window_balance_count (tree->child1, tree->split_horizontal) + 1;
        count_right = gui_window_balance_count (tree->child2, tree->split_horizontal) + 1;
        if (count_right > count_left)
            new_split_pct = (count_left * 100) / (count_left + count_right);
        else
            new_split_pct = (count_right * 100) / (count_left + count_right);
        if (new_split_pct < 1)
            new_split_pct = 1;
        if (new_split_pct > 99)
            new_split_pct = 99;
        if ((tree->split_horizontal && (count_right > count_left))
            || (!tree->split_horizontal && (count_left > count_right)))
            new_split_pct = 100 - new_split_pct;
        if (tree->split_pct != new_split_pct)
        {
            tree->split_pct = new_split_pct;
            balanced = 1;
        }
        balanced |= gui_window_balance (tree->child1);
        balanced |= gui_window_balance (tree->child2);
    }
    return balanced;
}

/*
 * Swaps buffers of two windows.
 *
 * Argument "direction" can be:
 *   0 = auto (swap with sister)
 *   1 = window above
 *   2 = window on the right
 *   3 = window below
 *   4 = window on the left
 */

void
gui_window_swap (struct t_gui_window *window, int direction)
{
    struct t_gui_window_tree *parent, *sister;
    struct t_gui_window *window2, *ptr_win;
    struct t_gui_buffer *buffer1;

    if (!window || !gui_init_ok)
        return;

    window2 = NULL;

    if (direction == 0)
    {
        /* search sister window */
        parent = window->ptr_tree->parent_node;
        if (parent)
        {
            sister = (parent->child1->window == window) ?
                parent->child2 : parent->child1;
            if (sister->window)
                window2 = sister->window;
        }
    }
    else
    {
        /* search window using direction */
        for (ptr_win = gui_windows; ptr_win;
             ptr_win = ptr_win->next_window)
        {
            if ((ptr_win != window) &&
                (gui_window_side_by_side (window, ptr_win) == direction))
            {
                window2 = ptr_win;
                break;
            }
        }
    }

    /* let's swap! */
    if (window2 && (window->buffer != window2->buffer))
    {
        buffer1 = window->buffer;
        gui_window_switch_to_buffer (window, window2->buffer, 0);
        gui_window_switch_to_buffer (window2, buffer1, 0);
    }
}

/*
 * Called when terminal size is modified.
 *
 * Argument full_refresh == 1 when ctrl-l is pressed, or if terminal is
 * resized.
 */

void
gui_window_refresh_screen (int full_refresh)
{
    if (full_refresh)
    {
        endwin ();
        refresh ();
        gui_window_read_terminal_size ();
        refresh ();
        gui_window_set_title (
            (CONFIG_STRING(config_look_window_title)
            && CONFIG_STRING(config_look_window_title)[0]) ?
            CONFIG_STRING(config_look_window_title) : NULL);
    }

    gui_window_refresh_windows ();
}

/*
 * Callback for bare display timer.
 */

int
gui_window_bare_display_timer_cb (const void *pointer, void *data,
                                  int remaining_calls)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (gui_window_bare_display)
        gui_window_bare_display_toggle (NULL);

    if (remaining_calls == 0)
        gui_window_bare_display_timer = NULL;

    return WEECHAT_RC_OK;
}

/*
 * Toggles bare display.
 */

void
gui_window_bare_display_toggle (const char *delay)
{
    long seconds;
    char *error;

    gui_window_bare_display ^= 1;

    if (gui_window_bare_display)
    {
        /* temporarily disable ncurses */
        endwin ();
        if (gui_mouse_enabled)
            gui_mouse_disable ();
        if (delay)
        {
            error = NULL;
            seconds = strtol (delay, &error, 10);
            if (error && !error[0] && (seconds >= 0))
            {
                if (gui_window_bare_display_timer)
                {
                    unhook (gui_window_bare_display_timer);
                    gui_window_bare_display_timer = NULL;
                }
                gui_window_bare_display_timer = hook_timer (
                    NULL,
                    seconds * 1000, 0, 1,
                    &gui_window_bare_display_timer_cb, NULL, NULL);
            }
        }
    }
    else
    {
        /* come back to standard display (with ncurses) */
        refresh ();
        if (gui_window_bare_display_timer)
        {
            unhook (gui_window_bare_display_timer);
            gui_window_bare_display_timer = NULL;
        }
        if (CONFIG_BOOLEAN(config_look_mouse))
            gui_mouse_enable ();
    }

    gui_window_ask_refresh (2);
}

/*
 * Sets terminal title.
 *
 * Note: the content of "title" (if not NULL) is evaluated, so variables like
 * "${info:version}" can be used inside.
 */

void
gui_window_set_title (const char *title)
{
    char *new_title, *envterm, *envshell, *shell, *shellname;

    envterm = getenv ("TERM");
    if (!envterm)
        return;

    new_title = (title && title[0]) ?
        eval_expression (title, NULL, NULL, NULL) : NULL;
    if (!new_title)
        return;

    if (strcmp (envterm, "sun-cmd") == 0)
    {
        printf ("\033]l%s\033\\", new_title);
    }
    else if (strcmp (envterm, "hpterm") == 0)
    {
        printf ("\033&f0k%dD%s", (int)(strlen (new_title) + 1), new_title);
    }
    else if ((strncmp (envterm, "screen", 6) == 0)
             || (strncmp (envterm, "tmux", 4) == 0))
    {
        if (title && title[0])
        {
            printf ("\033k%s\033\\", new_title);
        }
        else
        {
            envshell = getenv ("SHELL");
            if (envshell)
            {
                shell  = strdup (envshell);
                if (shell)
                {
                    shellname = basename (shell);
                    printf ("\033k%s\033\\", (shellname) ? shellname : shell);
                    free (shell);
                }
                else
                {
                    printf ("\033k%s\033\\", envterm);
                }
            }
            else
            {
                printf ("\033k%s\033\\", envterm);
            }
        }
        /* trying to set the title of a backgrounded xterm like terminal */
        printf ("\33]0;%s\7", new_title);
    }
    else
    {    /* we suppose all other terminals support the xterm escape codes */
        printf ("\33]0;%s\7", new_title);
    }

    fflush (stdout);

    free (new_title);
}

/*
 * Copies text to clipboard (sent to terminal).
 */

void
gui_window_send_clipboard (const char *storage_unit, const char *text)
{
    char *text_base64;
    int length;

    length = strlen (text);
    text_base64 = malloc ((length * 4) + 1);
    if (text_base64)
    {
        if (string_base64_encode (text, length, text_base64) >= 0)
        {
            fprintf (stderr, "\033]52;%s;%s\a",
                     (storage_unit) ? storage_unit : "",
                     text_base64);
            fflush (stderr);
        }
        free (text_base64);
    }
}

/*
 * Enables/disables bracketed paste mode.
 */

void
gui_window_set_bracketed_paste_mode (int enable)
{
    char *envterm, *envtmux;
    int tmux, screen;

    envtmux = getenv ("TMUX");
    tmux = (envtmux && envtmux[0]);

    envterm = getenv ("TERM");
    screen = (envterm && (strncmp (envterm, "screen", 6) == 0) && !tmux);

    fprintf (stderr, "%s\033[?2004%s%s",
             (screen) ? "\033P" : "",
             (enable) ? "h" : "l",
             (screen) ? "\033\\" : "");
    fflush (stderr);
}

/*
 * Moves cursor on screen (for cursor mode).
 */

void
gui_window_move_cursor ()
{
    if (gui_cursor_mode)
    {
        move (gui_cursor_y, gui_cursor_x);
        refresh ();
    }
}

/*
 * Displays some infos about terminal and colors.
 */

void
gui_window_term_display_infos ()
{
    gui_chat_printf (NULL, "");
    gui_chat_printf (NULL, _("Terminal infos:"));
    gui_chat_printf (NULL, _("  TERM='%s', size: %dx%d"),
                     getenv ("TERM"), gui_term_cols, gui_term_lines);
}

/*
 * Prints window Curses objects infos in WeeChat log file (usually for crash
 * dump).
 */

void
gui_window_objects_print_log (struct t_gui_window *window)
{
    log_printf ("  window specific objects for Curses:");
    log_printf ("    win_chat. . . . . . . : 0x%lx", GUI_WINDOW_OBJECTS(window)->win_chat);
    log_printf ("    win_separator_horiz . : 0x%lx", GUI_WINDOW_OBJECTS(window)->win_separator_horiz);
    log_printf ("    win_separator_vertic. : 0x%lx", GUI_WINDOW_OBJECTS(window)->win_separator_vertic);
}
