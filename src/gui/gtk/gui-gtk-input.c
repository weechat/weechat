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

/* gui-gtk-input: user input functions for Gtk GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-utf8.h"
#include "../../plugins/plugin.h"
#include "../gui-input.h"
#include "../gui-window.h"
#include "gui-gtk.h"


/*
 * gui_input_set_color: set color for an input window
 */

void
gui_input_set_color (struct t_gui_window *window, int irc_color)
{
    /*int fg, bg;*/

    /* TODO: write this function for Gtk */
    (void) window;
    (void) irc_color;
}

/*
 * gui_input_draw_prompt: display input prompt
 */

void
gui_input_draw_prompt (struct t_gui_window *window, char *nick)
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
gui_input_draw_text (struct t_gui_window *window, int input_width)
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
gui_input_draw (struct t_gui_buffer *buffer, int erase)
{
    /*struct t_gui_window *ptr_win;
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
