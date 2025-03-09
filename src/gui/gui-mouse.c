/*
 * gui-mouse.c - functions for mouse (used by all GUI)
 *
 * Copyright (C) 2011-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <string.h>

#include "../core/weechat.h"
#include "gui-chat.h"


int gui_mouse_enabled = 0;             /* 1 if mouse support is enabled     */
int gui_mouse_debug = 0;               /* debug mode for mouse (0-2)        */
int gui_mouse_grab = 0;                /* 1 if grab mode enabled            */

/* mouse event */
int gui_mouse_event_pending = 0;       /* 1 if mouse event has started      */
int gui_mouse_event_index = 0;         /* index for x/y in array (0 or 1)   */
int gui_mouse_event_x[2] = { 0, 0 };   /* position of latest mouse event:   */
                                       /* (on click, on release)            */
int gui_mouse_event_y[2] = { 0, 0 };   /* position of latest mouse event    */
                                       /* (on click, on release)            */
char gui_mouse_event_button = '#';     /* button pressed (or wheel)         */


/*
 * Sets debug for mouse events.
 */

void
gui_mouse_debug_set (int debug)
{
    gui_mouse_debug = debug;

    if (gui_mouse_debug)
    {
        gui_chat_printf (NULL, _("Debug enabled for mouse (%s)"),
                         (debug > 1) ? _("verbose") : _("normal"));
    }
    else
        gui_chat_printf (NULL, _("Debug disabled for mouse"));
}

/*
 * Resets event values.
 */

void
gui_mouse_event_reset (void)
{
    gui_mouse_event_index = 0;
    gui_mouse_event_x[0] = 0;
    gui_mouse_event_y[0] = 0;
    gui_mouse_event_x[1] = 0;
    gui_mouse_event_y[1] = 0;
    gui_mouse_event_button = '#';
}
