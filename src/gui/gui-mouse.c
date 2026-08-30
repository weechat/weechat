/*
 * SPDX-FileCopyrightText: 2011-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Functions for mouse (used by all GUI) */

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

/* Mouse event */
int gui_mouse_event_pending = 0;       /* 1 if mouse event has started      */
int gui_mouse_event_index = 0;         /* index for x/y in array (0 or 1)   */
int gui_mouse_event_x[2] = { 0, 0 };   /* position of latest mouse event:   */
                                       /* (on click, on release)            */
int gui_mouse_event_y[2] = { 0, 0 };   /* position of latest mouse event    */
                                       /* (on click, on release)            */
char gui_mouse_event_button = '#';     /* button pressed (or wheel)         */


/*
 * Set debug for mouse events.
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
 * Reset event values.
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
