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
 * gui-curses-mouse.c: mouse functions for Curses GUI
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-hook.h"
#include "../../core/wee-utf8.h"
#include "../../plugins/plugin.h"
#include "../gui-chat.h"
#include "../gui-key.h"
#include "../gui-mouse.h"


/*
 * gui_mouse_enable: enable mouse
 */

void
gui_mouse_enable ()
{
    gui_mouse_enabled = 1;
    fprintf (stderr, "\033[?1005h\033[?1000h\033[?1002h");
}

/*
 * gui_mouse_disable: disable mouse
 */

void
gui_mouse_disable ()
{
    gui_mouse_enabled = 0;
    fprintf (stderr, "\033[?1002l\033[?1000l\033[?1005l");
}

/*
 * gui_mouse_display_state: display state of mouse
 */

void
gui_mouse_display_state ()
{
    gui_chat_printf (NULL,
                     (CONFIG_BOOLEAN(config_look_mouse)) ?
                     _("Mouse is enabled") : _("Mouse is disabled"));
}

/*
 * gui_mouse_grab_timer_cb: timer for grabbing mouse code
 */

int
gui_mouse_grab_timer_cb (void *data, int remaining_calls)
{
    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;
    
    gui_mouse_grab_end ();
    
    return WEECHAT_RC_OK;
}

/*
 * gui_mouse_grab_init: init "grab mouse" mode
 */

void
gui_mouse_grab_init ()
{
    gui_mouse_grab = 1;
    
    if (gui_mouse_event_timer)
        unhook (gui_mouse_event_timer);
    
    gui_mouse_event_timer = hook_timer (NULL,
                                        CONFIG_INTEGER(config_look_mouse_timer_delay),
                                        0, 1,
                                        &gui_mouse_grab_timer_cb, NULL);
}

/*
 * gui_mouse_grab_code2key: get key name with a mouse code
 *                          *extra_chars is set with first char following the
 *                          end of mouse code (this can point to the '\0' or
 *                          other chars)
 */

const char *
gui_mouse_grab_code2key (const char *code, char **extra_chars)
{
    int x, y, code_utf8, length;
    double diff_x, diff_y, distance, angle, pi4;
    static char key[128];
    char button[2], *ptr_code;
    
    *extra_chars = NULL;
    
    key[0] = '\0';
    
    /*
     * mouse code must have at least:
     *   one code (for event) + X + Y == 3 bytes or 3 UTF-8 chars
     */
    code_utf8 = utf8_is_valid (code, NULL);
    length = (code_utf8) ? utf8_strlen (code) : (int)strlen (code);
    if (length < 3)
        return NULL;
    
    /* get coordinates and button */
    if (code_utf8)
    {
        /* get coordinates using UTF-8 chars in code */
        x = utf8_char_int (code + 1) - 33;
        ptr_code = utf8_next_char (code + 1);
        if (!ptr_code)
            return NULL;
        y = utf8_char_int (ptr_code) - 33;
        *extra_chars = utf8_next_char (ptr_code);
    }
    else
    {
        /* get coordinates using ISO chars in code */
        x = ((unsigned char)code[1]) - 33;
        y = ((unsigned char)code[2]) - 33;
        *extra_chars = (char *)code + 3;
    }
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    
    /* ignore code '#' (button released) if it's received as first event */
    if ((gui_mouse_event_index == 0) && (code[0] == '#'))
        return NULL;
    
    /* set data in "gui_mouse_event_xxx" */
    gui_mouse_event_x[gui_mouse_event_index] = x;
    gui_mouse_event_y[gui_mouse_event_index] = y;
    if (gui_mouse_event_index == 0)
        gui_mouse_event_button = code[0];
    
    if (gui_mouse_event_index == 0)
        gui_mouse_event_index = 1;
    
    if (code[0] == '`')
    {
        strcat (key, "wheelup");
        return key;
    }
    
    if (code[0] == 'a')
    {
        strcat (key, "wheeldown");
        return key;
    }
    
    if (code[0] != '#')
        return NULL;
    
    /* add button/wheel */
    switch (gui_mouse_event_button)
    {
        case ' ': /* left button pressed */
            strcat (key, "button1");
            break;
        case '"': /* right button pressed */
            strcat (key, "button2");
            break;
        case '!': /* middle button pressed */
            strcat (key, "button3");
            break;
        default: /* extra buttons: button4..button9 */
            if ((gui_mouse_event_button >= 'b')
                && (gui_mouse_event_button <= 'g'))
            {
                button[0] = gui_mouse_event_button - ('b' - '4');
                button[1] = '\0';
                strcat (key, "button");
                strcat (key, button);
            }
            break;
    }
    
    /*
     * Mouse gesture: if (x,y) on release is different from (x,y) on click,
     * compute distance and angle between 2 points.
     *
     * Distance: sqrt((x2-x1)²+(y2-y1)²)
     * Angle   : atan2(x1-x1, y2-y1)
     * 
     * Angle:
     * 
     *              3.14             pi
     *               /\
     *       -2.35   ||   2.35       3/4 * pi
     *               ||
     *   -1.57  /----++----\  1.57   1/2 * pi
     *          \----++----/
     *               ||
     *       -0.78   ||   0.78       1/4 * pi
     *               \/
     *              0.00             0
     *
     * Possible returned gestures are:
     *
     *   key name                   | dist. | angle
     *   ---------------------------+-------+--------------------------
     *   buttonX-gesture-up         | 3..19 | -2.35..-3.14 + 2.35..3.14
     *   buttonX-gesture-up-long    | >= 20 |
     *   buttonX-gesture-down       | 3..19 | -0.78..0.78
     *   buttonX-gesture-down-long  | >= 20 |
     *   buttonX-gesture-left       | 3..39 | -0.78..-2.35
     *   buttonX-gesture-left-long  | >= 40 |
     *   buttonX-gesture-right      | 3..39 |  0.78..2.35
     *   buttonX-gesture-right-long | >= 40 |
     */
    
    distance = 0;
    if (key[0]
        && ((gui_mouse_event_x[0] != gui_mouse_event_x[1])
            || (gui_mouse_event_y[0] != gui_mouse_event_y[1])))
    {
        diff_x = gui_mouse_event_x[1] - gui_mouse_event_x[0];
        diff_y = gui_mouse_event_y[1] - gui_mouse_event_y[0];
        distance = sqrt ((diff_x * diff_x) + (diff_y * diff_y));
        if (distance >= 3)
        {
            angle = atan2 ((double)(gui_mouse_event_x[1] - gui_mouse_event_x[0]),
                           (double)(gui_mouse_event_y[1] - gui_mouse_event_y[0]));
            pi4 = 3.14159265358979 / 4;
            if ((angle <= pi4 * (-3)) || (angle >= pi4 * 3))
            {
                strcat (key, "-gesture-up");
                if (distance >= 20)
                    strcat (key, "-long");
            }
            else if ((angle >= pi4 * (-1)) && (angle <= pi4))
            {
                strcat (key, "-gesture-down");
                if (distance >= 20)
                    strcat (key, "-long");
            }
            else if ((angle >= pi4 * (-3)) && (angle <= pi4 * (-1)))
            {
                strcat (key, "-gesture-left");
                if (distance >= 40)
                    strcat (key, "-long");
            }
            else if ((angle >= pi4) && (angle <= pi4 * 3))
            {
                strcat (key, "-gesture-right");
                if (distance >= 40)
                    strcat (key, "-long");
            }
        }
    }
    
    return key;
}

/*
 * gui_mouse_grab_end: end "grab mouse" mode
 */

void
gui_mouse_grab_end ()
{
    const char *mouse_key;
    char *extra_chars;
    int i;
    
    gui_mouse_grab = 0;
    
    /* end mouse event timer */
    if (gui_mouse_event_timer)
    {
        unhook (gui_mouse_event_timer);
        gui_mouse_event_timer = NULL;
    }
    
    /* get key from mouse code and execute command (if found) */
    mouse_key = gui_mouse_grab_code2key (gui_key_combo_buffer, &extra_chars);
    if (mouse_key && mouse_key[0])
    {
        (void) gui_key_focus (mouse_key,
                              GUI_KEY_CONTEXT_MOUSE);
        gui_mouse_reset_event ();
    }
    
    gui_key_combo_buffer[0] = '\0';
    
    /*
     * if extra chars, use them as new input (this can happen if used typed
     * something and that mouse timer was not reached yet
     */
    if (extra_chars && extra_chars[0])
    {
        for (i = 0; extra_chars[i]; i++)
        {
            gui_key_buffer_add ((unsigned char)extra_chars[i]);
        }
        gui_key_flush ();
    }
}
