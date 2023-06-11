/*
 * gui-curses-mouse.c - mouse functions for Curses GUI
 *
 * Copyright (C) 2011-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-hook.h"
#include "../../core/wee-string.h"
#include "../../core/wee-utf8.h"
#include "../../plugins/plugin.h"
#include "../gui-bar.h"
#include "../gui-bar-window.h"
#include "../gui-buffer.h"
#include "../gui-chat.h"
#include "../gui-completion.h"
#include "../gui-focus.h"
#include "../gui-input.h"
#include "../gui-key.h"
#include "../gui-mouse.h"
#include "../gui-window.h"
#include "gui-curses-mouse.h"


char *gui_mouse_wheel_codes[][2] =
{ { "`",  "wheelup"            },
  { "p",  "ctrl-wheelup"       },
  { "h",  "alt-wheelup"        },
  { "x",  "alt-ctrl-wheelup"   },
  { "a",  "wheeldown"          },
  { "q",  "ctrl-wheeldown"     },
  { "i",  "alt-wheeldown"      },
  { "y",  "alt-ctrl-wheeldown" },
  { NULL, NULL                 } };

char *gui_mouse_button_codes[][2] =
{ { " ",  "button1"            },
  { "\"", "button2"            },
  { "!",  "button3"            },
  { "b",  "button4"            },
  { "c",  "button5"            },
  { "d",  "button6"            },
  { "e",  "button7"            },
  { "f",  "button8"            },
  { "g",  "button9"            },
  { "0",  "ctrl-button1"       },
  { "2",  "ctrl-button2"       },
  { "1",  "ctrl-button3"       },
  { "(",  "alt-button1"        },
  { "*",  "alt-button2"        },
  { ")",  "alt-button3"        },
  { "8",  "alt-ctrl-button1"   },
  { NULL, NULL                 } };


/*
 * Enables mouse.
 */

void
gui_mouse_enable ()
{
    gui_mouse_enabled = 1;
    fprintf (stderr, "\033[?1005h\033[?1000h\033[?1002h");
    fflush (stderr);

    (void) hook_signal_send ("mouse_enabled",
                             WEECHAT_HOOK_SIGNAL_STRING, NULL);
}

/*
 * Disables mouse.
 */

void
gui_mouse_disable ()
{
    gui_mouse_enabled = 0;
    fprintf (stderr, "\033[?1002l\033[?1000l\033[?1005l");
    fflush (stderr);

    (void) hook_signal_send ("mouse_disabled",
                             WEECHAT_HOOK_SIGNAL_STRING, NULL);
}

/*
 * Displays state of mouse.
 */

void
gui_mouse_display_state ()
{
    gui_chat_printf (NULL,
                     gui_mouse_enabled ?
                     _("Mouse is enabled") : _("Mouse is disabled"));
}

/*
 * Initializes "grab mode".
 */

void
gui_mouse_grab_init (int area)
{
    gui_mouse_grab = (area) ? 2 : 1;
}

/*
 * Gets area for input, according to (x,y) of mouse event.
 *
 * For example: @item(buffer_nicklist)
 *              @bar(title)
 *              @chat
 *              @*
 */

const char *
gui_mouse_grab_event2input ()
{
    struct t_gui_focus_info *focus_info;
    static char area[256];

    area[0] = '\0';

    focus_info = gui_focus_get_info (gui_mouse_event_x[0],
                                     gui_mouse_event_y[0]);
    if (focus_info)
    {
        if (focus_info->bar_item)
        {
            snprintf (area, sizeof (area),
                      "@item(%s)", focus_info->bar_item);
        }
        else if (focus_info->bar_window)
        {
            snprintf (area, sizeof (area),
                      "@bar(%s)", ((focus_info->bar_window)->bar)->name);
        }
        else if (focus_info->chat)
        {
            snprintf (area, sizeof (area), "@chat");
        }
        else
        {
            snprintf (area, sizeof (area), "@*");
        }
        gui_focus_free_info (focus_info);
    }

    return area;
}

/*
 * Ends "grab mode".
 */

void
gui_mouse_grab_end (const char *mouse_key)
{
    char mouse_key_input[256];

    /* insert mouse key in input */
    if (gui_current_window->buffer->input)
    {
        if (gui_mouse_grab == 2)
        {
            /* mouse key with area */
            snprintf (mouse_key_input, sizeof (mouse_key_input),
                      "%s:%s",
                      gui_mouse_grab_event2input (),
                      mouse_key);
        }
        else
        {
            /* mouse key without area */
            snprintf (mouse_key_input, sizeof (mouse_key_input),
                      "%s", mouse_key);
        }
        gui_input_insert_string (gui_current_window->buffer, mouse_key_input);
        gui_input_text_changed_modifier_and_signal (gui_current_window->buffer,
                                                    1, /* save undo */
                                                    1); /* stop completion */
    }

    gui_mouse_grab = 0;
}

/*
 * Timer for grabbing mouse code.
 */

int
gui_mouse_event_timer_cb (const void *pointer, void *data, int remaining_calls)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) remaining_calls;

    gui_mouse_event_end ();

    return WEECHAT_RC_OK;
}

/*
 * Initializes mouse event.
 */

void
gui_mouse_event_init ()
{
    gui_mouse_event_pending = 1;

    if (gui_mouse_event_timer)
        unhook (gui_mouse_event_timer);

    gui_mouse_event_timer = hook_timer (NULL,
                                        CONFIG_INTEGER(config_look_mouse_timer_delay),
                                        0, 1,
                                        &gui_mouse_event_timer_cb, NULL, NULL);
}

/*
 * Gets key name with a mouse code.
 */

const char *
gui_mouse_event_code2key (const char *code)
{
    int i, x, y, code_utf8, length;
    double diff_x, diff_y, distance, angle, pi4;
    static char key[128];
    const char *ptr_code;

    key[0] = '\0';

    /*
     * mouse code must have at least:
     *   one code (for event) + X + Y == 3 bytes or 3 UTF-8 chars
     */
    code_utf8 = utf8_is_valid (code, -1, NULL);
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
    }
    else
    {
        /* get coordinates using ISO chars in code */
        x = ((unsigned char)code[1]) - 33;
        y = ((unsigned char)code[2]) - 33;
    }
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;

    /* ignore code if it's motion/end code received as first event */
    if ((gui_mouse_event_index == 0)
        && (MOUSE_CODE_MOTION(code[0]) || MOUSE_CODE_END(code[0])))
    {
        return NULL;
    }

    /* set data in "gui_mouse_event_xxx" */
    gui_mouse_event_x[gui_mouse_event_index] = x;
    gui_mouse_event_y[gui_mouse_event_index] = y;
    if (gui_mouse_event_index == 0)
        gui_mouse_event_button = code[0];

    if (gui_mouse_event_index == 0)
        gui_mouse_event_index = 1;

    /*
     * browse wheel codes, if one code is found, return event name immediately
     */
    for (i = 0; gui_mouse_wheel_codes[i][0]; i++)
    {
        if (code[0] == gui_mouse_wheel_codes[i][0][0])
        {
            strcat (key, gui_mouse_wheel_codes[i][1]);
            gui_mouse_event_x[1] = gui_mouse_event_x[0];
            gui_mouse_event_y[1] = gui_mouse_event_y[0];
            return key;
        }
    }

    /* add name of button event */
    for (i = 0; gui_mouse_button_codes[i][0]; i++)
    {
        if (gui_mouse_event_button == gui_mouse_button_codes[i][0][0])
        {
            strcat (key, gui_mouse_button_codes[i][1]);
            break;
        }
    }

    /* nothing found, reset now or mouse will be stuck */
    if (!key[0])
    {
        gui_mouse_event_reset ();
        return NULL;
    }

    if (!MOUSE_CODE_END(code[0]))
    {
        strcat (key, "-event-");
        if (MOUSE_CODE_MOTION(code[0])) {
            strcat (key, "drag");
        }
        else
        {
            gui_mouse_event_x[1] = gui_mouse_event_x[0];
            gui_mouse_event_y[1] = gui_mouse_event_y[0];
            strcat (key, "down");
        }
        return key;
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
 * Ends mouse event.
 */

void
gui_mouse_event_end ()
{
    const char *mouse_key;
    int bare_event;

    if (gui_key_debug)
        gui_key_debug_print_key (gui_key_combo, NULL, NULL, NULL, 1);

    gui_mouse_event_pending = 0;

    /* end mouse event timer */
    if (gui_mouse_event_timer)
    {
        unhook (gui_mouse_event_timer);
        gui_mouse_event_timer = NULL;
    }

    /* get key from mouse code */
    mouse_key = gui_mouse_event_code2key (gui_key_combo);
    if (mouse_key && mouse_key[0])
    {
        bare_event = string_match (mouse_key, "*-event-*", 1);
        if (gui_mouse_grab)
        {
            if (!bare_event)
                gui_mouse_grab_end (mouse_key);
        }
        else if (!gui_key_debug)
        {
            /* execute command (if found) */
            (void) gui_key_focus (mouse_key,
                                  GUI_KEY_CONTEXT_MOUSE);
        }
        if (!bare_event)
            gui_mouse_event_reset ();
    }

    gui_key_combo[0] = '\0';
}
