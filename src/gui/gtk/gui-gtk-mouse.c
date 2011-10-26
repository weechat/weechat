/*
 * Copyright (C) 2011 Sebastien Helleu <flashcode@flashtux.org>
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

/*
 * gui-gtk-mouse.c: mouse functions for Gtk GUI
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../../core/weechat.h"
#include "../gui-mouse.h"


/*
 * gui_mouse_enable: enable mouse
 */

void
gui_mouse_enable ()
{
    /* This function does nothing in Gtk GUI */
}

/*
 * gui_mouse_disable: disable mouse
 */

void
gui_mouse_disable ()
{
    /* This function does nothing in Gtk GUI */
}

/*
 * gui_mouse_display_state: display state of mouse
 */

void
gui_mouse_display_state ()
{
    /* This function does nothing in Gtk GUI */
}

/*
 * gui_mouse_grab_init: init "grab mode"
 */

void
gui_mouse_grab_init (int area)
{
    (void) area;

    /* This function does nothing in Gtk GUI */
}

/*
 * gui_mouse_event_init: init mouse event
 */

void
gui_mouse_event_init ()
{
    /* This function does nothing in Gtk GUI */
}

/*
 * gui_mouse_event_code2key: get key name with a mouse code
 */

const char *
gui_mouse_event_code2key (const char *code)
{
    (void) code;

    /* This function does nothing in Gtk GUI */

    return NULL;
}

/*
 * gui_mouse_event_end: end mouse event
 */

void
gui_mouse_event_end ()
{
    /* This function does nothing in Gtk GUI */
}
