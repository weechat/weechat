/*
 * gui-gtk-mouse.c - mouse functions for Gtk GUI
 *
 * Copyright (C) 2011-2012 Sebastien Helleu <flashcode@flashtux.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../../core/weechat.h"
#include "../gui-mouse.h"


/*
 * Enables mouse.
 */

void
gui_mouse_enable ()
{
    /* This function does nothing in Gtk GUI */
}

/*
 * Disables mouse.
 */

void
gui_mouse_disable ()
{
    /* This function does nothing in Gtk GUI */
}

/*
 * Displays state of mouse.
 */

void
gui_mouse_display_state ()
{
    /* This function does nothing in Gtk GUI */
}

/*
 * Initializes "grab mode".
 */

void
gui_mouse_grab_init (int area)
{
    (void) area;

    /* This function does nothing in Gtk GUI */
}

/*
 * Initializes mouse event.
 */

void
gui_mouse_event_init ()
{
    /* This function does nothing in Gtk GUI */
}

/*
 * Gets key name with a mouse code.
 */

const char *
gui_mouse_event_code2key (const char *code)
{
    (void) code;

    /* This function does nothing in Gtk GUI */

    return NULL;
}

/*
 * Ends mouse event.
 */

void
gui_mouse_event_end ()
{
    /* This function does nothing in Gtk GUI */
}
