/*
 * gui-curses-mouse.c - mouse functions for Curses GUI
 *
 * Copyright (C) 2011-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <ctype.h>
#include <math.h>

#include "../../core/weechat.h"
#include "../../core/core-config.h"
#include "../../core/core-hook.h"
#include "../../core/core-string.h"
#include "../../core/core-utf8.h"
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


char *gui_mouse_wheel_utf8_codes[][2] =
{ { "`",  "wheelup"            },
  { "p",  "ctrl-wheelup"       },
  { "h",  "alt-wheelup"        },
  { "x",  "alt-ctrl-wheelup"   },
  { "a",  "wheeldown"          },
  { "q",  "ctrl-wheeldown"     },
  { "i",  "alt-wheeldown"      },
  { "y",  "alt-ctrl-wheeldown" },
  { NULL, NULL                 } };

char *gui_mouse_button_utf8_codes[][2] =
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
  { ":",  "alt-ctrl-button2"   },
  { "9",  "alt-ctrl-button3"   },
  { NULL, NULL                 } };


/*
 * Enables mouse.
 */

void
gui_mouse_enable ()
{
    gui_mouse_enabled = 1;
    fprintf (stderr, "\033[?1005h\033[?1006h\033[?1000h\033[?1002h");
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
    fprintf (stderr, "\033[?1002l\033[?1000l\033[?1006l\033[?1005l");
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
 * Returns size of mouse event (SGR and UTF-8 events are supported):
 *     -1: not a mouse event
 *      0: incomplete mouse event
 *    > 0: complete mouse event
 */

int
gui_mouse_event_size (const char *key)
{
    const char *ptr_key;
    int i, key_valid_utf8;

    if (!key || !key[0])
        return -1;

    if (strncmp (key, "\x01[[<", 4) == 0)
    {
        /*
         * SGR event: digits separated by ';', ending by 'M' (pressed)
         * or 'm' (released), example: "0;71;21M"
         */
        for (ptr_key = key + 4; ptr_key[0]; ptr_key++)
        {
            if ((ptr_key[0] != ';') && !isdigit ((unsigned char)ptr_key[0]))
                return ptr_key - key + 1;
        }
        return 0;
    }
    else if (strncmp (key, "\x01[[M", 4) == 0)
    {
        /* UTF-8 event: 3 UTF-8 chars, example: "!4&" */
        ptr_key = key + 4;
        key_valid_utf8 = utf8_is_valid (ptr_key, -1, NULL);
        for (i = 0; i < 3; i++)
        {
            if (!ptr_key[0])
                return 0;
            ptr_key = (key_valid_utf8) ? utf8_next_char (ptr_key) : ptr_key + 1;
            if (!ptr_key)
                return 0;
        }
        return ptr_key - key;
    }

    /* invalid mouse event, or not supported */
    return -1;
}

/*
 * Concatenates the mouse event gesture in a key (containing the button name).
 *
 * Note: *key must be long enough for the gesture added by this function
 * (see below).
 *
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
 *   value returned        | dist. | angle
 *   ----------------------+-------+--------------------------
 *   "-gesture-up"         | 3..19 | -2.35..-3.14 + 2.35..3.14
 *   "-gesture-up-long"    | >= 20 |
 *   "-gesture-down"       | 3..19 | -0.78..0.78
 *   "-gesture-down-long"  | >= 20 |
 *   "-gesture-left"       | 3..39 | -0.78..-2.35
 *   "-gesture-left-long"  | >= 40 |
 *   "-gesture-right"      | 3..39 |  0.78..2.35
 *   "-gesture-right-long" | >= 40 |
 */

void
gui_mouse_event_concat_gesture (char *key)
{
    double diff_x, diff_y, distance, angle, pi4;

    /* (x, y) == (x2, y2) => no gesture, do not add anything */
    if ((gui_mouse_event_x[0] == gui_mouse_event_x[1])
        && (gui_mouse_event_y[0] == gui_mouse_event_y[1]))
        return;

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

/*
 * Gets mouse event name with a SGR mouse event.
 */

const char *
gui_mouse_event_name_sgr (const char *key)
{
    int length, num_items, is_release;
    char **items, *error;
    static char mouse_key[128];
    long button, x, y;

    if (!key || !key[0])
        return NULL;

    mouse_key[0] = '\0';

    items = string_split (key, ";", NULL, 0, 0, &num_items);
    if (!items)
        return NULL;

    if (num_items < 3)
        goto error;

    error = NULL;
    button = strtol (items[0], &error, 10);
    if (!error || error[0])
        goto error;

    error = NULL;
    x = strtol (items[1], &error, 10);
    if (!error || error[0])
        goto error;
    x = (x >= 1) ? x - 1 : 0;

    length = (int)strlen (items[2]);
    if (length <= 0)
        goto error;
    is_release = (items[2][length - 1] == 'm') ? 1 : 0;
    items[2][length - 1] = '\0';
    error = NULL;
    y = strtol (items[2], &error, 10);
    if (!error || error[0])
        goto error;
    y = (y >= 1) ? y - 1 : 0;

    /* set data in "gui_mouse_event_xxx" */
    gui_mouse_event_x[gui_mouse_event_index] = x;
    gui_mouse_event_y[gui_mouse_event_index] = y;

    /* keep same coordinates if it's release code received as first event */
    if ((gui_mouse_event_index == 0) && is_release)
    {
        gui_mouse_event_index = 1;
        gui_mouse_event_x[1] = gui_mouse_event_x[0];
        gui_mouse_event_y[1] = gui_mouse_event_y[0];
    }

    if (gui_mouse_event_index == 0)
        gui_mouse_event_index = 1;

    if (button & 8)
        strcat (mouse_key, "alt-");
    if (button & 16)
        strcat (mouse_key, "ctrl-");
    if (button & 4)
        strcat (mouse_key, "shift-");

    if (button & 64)
    {
        if ((button & 3) == 0)
            strcat (mouse_key, "wheelup");
        else if ((button & 3) == 1)
            strcat (mouse_key, "wheeldown");
        /* not yet supported: wheel left/right */
        /*
        else if ((button & 3) == 2)
            strcat (mouse_key, "wheelleft");
        else if ((button & 3) == 3)
            strcat (mouse_key, "wheelright");
        */
        gui_mouse_event_x[1] = gui_mouse_event_x[0];
        gui_mouse_event_y[1] = gui_mouse_event_y[0];
        goto end;
    }
    else if (button & 128)
    {
        if ((button & 3) == 0)
            strcat (mouse_key, "button8");
        else if ((button & 3) == 1)
            strcat (mouse_key, "button9");
        else if ((button & 3) == 2)
            strcat (mouse_key, "button10");
        else if ((button & 3) == 3)
            strcat (mouse_key, "button11");
    }
    else
    {
        if ((button & 3) == 0)
            strcat (mouse_key, "button1");
        else if ((button & 3) == 1)
            strcat (mouse_key, "button3");
        else if ((button & 3) == 2)
            strcat (mouse_key, "button2");
    }

    if (!is_release && !(button & 64))
    {
        strcat (mouse_key, "-event-");
        if (button & 32)
        {
            strcat (mouse_key, "drag");
        }
        else
        {
            gui_mouse_event_x[1] = gui_mouse_event_x[0];
            gui_mouse_event_y[1] = gui_mouse_event_y[0];
            strcat (mouse_key, "down");
        }
        goto end;
    }

    gui_mouse_event_concat_gesture (mouse_key);

end:
    string_free_split (items);
    return mouse_key;

error:
    string_free_split (items);
    return NULL;
}

/*
 * Gets mouse event name with a UTF-8 mouse event: if the key is invalid UTF-8,
 * use the 3 bytes, otherwise 3 UTF-8 chars.
 */

const char *
gui_mouse_event_name_utf8 (const char *key)
{
    int i, x, y, key_valid_utf8, length;
    static char mouse_key[128];
    const char *ptr_key;

    if (!key || !key[0])
        return NULL;

    mouse_key[0] = '\0';

    /*
     * key must have at least:
     *   one code (for event) + X + Y == 3 bytes or 3 UTF-8 chars
     */
    key_valid_utf8 = utf8_is_valid (key, -1, NULL);
    length = (key_valid_utf8) ? utf8_strlen (key) : (int)strlen (key);
    if (length < 3)
        return NULL;

    /* get coordinates and button */
    if (key_valid_utf8)
    {
        /* get coordinates using UTF-8 chars in key */
        x = utf8_char_int (key + 1) - 33;
        ptr_key = (char *)utf8_next_char (key + 1);
        if (!ptr_key)
            return NULL;
        y = utf8_char_int (ptr_key) - 33;
    }
    else
    {
        /* get coordinates using ISO chars in key */
        x = ((unsigned char)key[1]) - 33;
        y = ((unsigned char)key[2]) - 33;
    }
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;

    /* ignore key if it's motion/end code received as first event */
    if ((gui_mouse_event_index == 0)
        && (MOUSE_CODE_UTF8_MOTION(key[0]) || MOUSE_CODE_UTF8_END(key[0])))
    {
        return NULL;
    }

    /* set data in "gui_mouse_event_xxx" */
    gui_mouse_event_x[gui_mouse_event_index] = x;
    gui_mouse_event_y[gui_mouse_event_index] = y;
    if (gui_mouse_event_index == 0)
        gui_mouse_event_button = key[0];

    if (gui_mouse_event_index == 0)
        gui_mouse_event_index = 1;

    /*
     * browse wheel codes, if one code is found, return event name immediately
     */
    for (i = 0; gui_mouse_wheel_utf8_codes[i][0]; i++)
    {
        if (key[0] == gui_mouse_wheel_utf8_codes[i][0][0])
        {
            strcat (mouse_key, gui_mouse_wheel_utf8_codes[i][1]);
            gui_mouse_event_x[1] = gui_mouse_event_x[0];
            gui_mouse_event_y[1] = gui_mouse_event_y[0];
            return mouse_key;
        }
    }

    /* add name of button event */
    for (i = 0; gui_mouse_button_utf8_codes[i][0]; i++)
    {
        if (gui_mouse_event_button == gui_mouse_button_utf8_codes[i][0][0])
        {
            strcat (mouse_key, gui_mouse_button_utf8_codes[i][1]);
            break;
        }
    }

    /* nothing found, reset now or mouse will be stuck */
    if (!mouse_key[0])
    {
        gui_mouse_event_reset ();
        return NULL;
    }

    if (!MOUSE_CODE_UTF8_END(key[0]))
    {
        strcat (mouse_key, "-event-");
        if (MOUSE_CODE_UTF8_MOTION(key[0])) {
            strcat (mouse_key, "drag");
        }
        else
        {
            gui_mouse_event_x[1] = gui_mouse_event_x[0];
            gui_mouse_event_y[1] = gui_mouse_event_y[0];
            strcat (mouse_key, "down");
        }
        return mouse_key;
    }

    gui_mouse_event_concat_gesture (mouse_key);

    return mouse_key;
}

/*
 * Processes a mouse event.
 */

void
gui_mouse_event_process (const char *key)
{
    const char *mouse_key;
    int bare_event;

    /* get mouse event name */
    mouse_key = NULL;
    if (strncmp (key, "\x01[[<", 4) == 0)
        mouse_key = gui_mouse_event_name_sgr (key + 4);
    else if (strncmp (key, "\x01[[M", 4) == 0)
        mouse_key = gui_mouse_event_name_utf8 (key + 4);

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
            (void) gui_key_focus (mouse_key, GUI_KEY_CONTEXT_MOUSE);
        }
        if (!bare_event)
            gui_mouse_event_reset ();
    }
}
