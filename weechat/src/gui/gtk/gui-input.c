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

/* gui-input: user input functions for Gtk GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include <gtk/gtk.h>

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../common/utf8.h"
#include "../../common/weeconfig.h"
#include "gui-gtk.h"

#ifdef PLUGINS
#include "../../plugins/plugins.h"
#endif


/*
 * gui_input_set_color: set color for an input window
 */

void
gui_input_set_color (t_gui_window *window, int irc_color)
{
    /*int fg, bg;*/

    /* TODO: write this function for Gtk */
    (void) window;
    (void) irc_color;
}

/*
 * gui_input_get_prompt_length: return input prompt length
 */

int
gui_input_get_prompt_length (t_gui_window *window, char *nick)
{
    char *pos, *modes;
    int length, mode_found;
    
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
                            length += utf8_strlen (CHANNEL(window->buffer)->name);
                        else
                        {
                            if (SERVER(window->buffer))
                                length += utf8_strlen (SERVER(window->buffer)->name);
                        }
                        pos++;
                        break;
                    case 'm':
                        if (SERVER(window->buffer))
                        {
                            mode_found = 0;
                            for (modes = SERVER(window->buffer)->nick_modes;
                                 modes && modes[0]; modes++)
                            {
                                if (modes[0] != ' ')
                                {
                                    length++;
                                    mode_found = 1;
                                }
                            }
                            if (mode_found)
                                length++;
                        }
                        pos++;
                        break;
                    case 'n':
                        length += utf8_strlen (nick);
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
                length++;
                pos += utf8_char_size (pos);
                break;
        }
    }
    return length;
}

/*
 * gui_input_draw_prompt: display input prompt
 */

void
gui_input_draw_prompt (t_gui_window *window, char *nick)
{
    /*char *pos, saved_char, *modes;
      int char_size, mode_found;*/

    /* TODO: write this function for Gtk */
    (void) window;
    (void) nick;
}

/*
 * gui_input_draw_text: display text in input buffer, according to color mask
 */

void
gui_input_draw_text (t_gui_window *window, int input_width)
{
    /*char *ptr_start, *ptr_next, saved_char;
      int pos_mask, size, last_color, color;*/

    /* TODO: write this function for Gtk */
    (void) window;
    (void) input_width;
}

/*
 * gui_input_draw: draw input window for a buffer
 */

void
gui_input_draw (t_gui_buffer *buffer, int erase)
{
    /*t_gui_window *ptr_win;
    char format[32];
    char *ptr_nickname;
    int input_width;
    t_irc_dcc *dcc_selected;*/
    
    if (!gui_ok)
        return;
    
    /* TODO: write this function for Gtk */
    (void) buffer;
    (void) erase;
}
