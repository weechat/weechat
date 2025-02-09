/*
 * gui-curses-key.c - keyboard functions for Curses GUI
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../core/weechat.h"
#include "../../core/core-config.h"
#include "../../core/core-hook.h"
#include "../../core/core-log.h"
#include "../../core/core-utf8.h"
#include "../../core/core-string.h"
#include "../../plugins/plugin.h"
#include "../gui-key.h"
#include "../gui-buffer.h"
#include "../gui-color.h"
#include "../gui-cursor.h"
#include "../gui-completion.h"
#include "../gui-input.h"
#include "../gui-mouse.h"
#include "../gui-window.h"
#include "gui-curses.h"

#define BIND(key, command)                                      \
    gui_key_default_bind (context, key, command, create_option)


/*
 * Creates key bind, only if it does not exist yet.
 */

void
gui_key_default_bind (int context, const char *key, const char *command,
                      int create_option)
{
    struct t_gui_key *ptr_key;

    ptr_key = gui_key_search (gui_keys[context], key);
    if (!ptr_key)
        gui_key_new (NULL, context, key, command, create_option);
}

/*
 * Creates default key bindings for a given context.
 *
 * If create_option == 1, config options are created, otherwise keys are just
 * added to linked list (gui_keys[]).
 */

void
gui_key_default_bindings (int context, int create_option)
{
    int i;
    char key_str[32], command[32];

    if (context == GUI_KEY_CONTEXT_DEFAULT)
    {
        BIND("return",            "/input return");
        BIND("meta-return",       "/input insert \\n");
        BIND("tab",               "/input complete_next");
        BIND("shift-tab",         "/input complete_previous");
        BIND("ctrl-r",            "/input search_history");
        BIND("ctrl-s",            "/input search_text_here");
        BIND("backspace",         "/input delete_previous_char");
        BIND("ctrl-_",            "/input undo");
        BIND("meta-_",            "/input redo");
        BIND("delete",            "/input delete_next_char");
        BIND("ctrl-d",            "/input delete_next_char");
        BIND("ctrl-w",            "/input delete_previous_word_whitespace");
        BIND("meta-backspace",    "/input delete_previous_word");
        BIND("ctrl-x",            "/buffer switch");
        BIND("meta-x",            "/buffer zoom");
        BIND("meta-d",            "/input delete_next_word");
        BIND("ctrl-k",            "/input delete_end_of_line");
        BIND("meta-ctrl-k",       "/input delete_end_of_input");
        BIND("meta-ctrl-l",       "/remote togglecmd");
        BIND("meta-r",            "/input delete_line");
        BIND("meta-R",            "/input delete_input");
        BIND("ctrl-t",            "/input transpose_chars");
        BIND("ctrl-u",            "/input delete_beginning_of_line");
        BIND("meta-ctrl-u",       "/input delete_beginning_of_input");
        BIND("ctrl-y",            "/input clipboard_paste");
        BIND("ctrl-z",            "/sys suspend");
        BIND("home",              "/input move_beginning_of_line");
        BIND("ctrl-a",            "/input move_beginning_of_line");
        BIND("shift-home",        "/input move_beginning_of_input");
        BIND("end",               "/input move_end_of_line");
        BIND("ctrl-e",            "/input move_end_of_line");
        BIND("shift-end",         "/input move_end_of_input");
        BIND("left",              "/input move_previous_char");
        BIND("shift-left",        "/input move_previous_char");
        BIND("ctrl-b",            "/input move_previous_char");
        BIND("right",             "/input move_next_char");
        BIND("shift-right",       "/input move_next_char");
        BIND("ctrl-f",            "/input move_next_char");
        BIND("meta-b",            "/input move_previous_word");
        BIND("ctrl-left",         "/input move_previous_word");
        BIND("meta-f",            "/input move_next_word");
        BIND("ctrl-right",        "/input move_next_word");
        BIND("up",                "/input history_previous");
        BIND("down",              "/input history_next");
        BIND("ctrl-up",           "/input history_global_previous");
        BIND("ctrl-down",         "/input history_global_next");
        BIND("ctrl-o",            "/input history_use_get_next");
        BIND("shift-up",          "/input move_previous_line");
        BIND("shift-down",        "/input move_next_line");
        BIND("meta-a",            "/buffer jump smart");
        BIND("meta-j,meta-f",     "/buffer -");
        BIND("meta-j,meta-l",     "/buffer +");
        BIND("meta-j,meta-r",     "/server raw");
        BIND("meta-j,meta-s",     "/server jump");
        BIND("meta-h,meta-c",     "/hotlist clear");
        BIND("meta-h,meta-m",     "/hotlist remove");
        BIND("meta-h,meta-r",     "/hotlist restore");
        BIND("meta-h,meta-R",     "/hotlist restore -all");
        BIND("meta-k",            "/input grab_key_command");
        BIND("meta-s",            "/mute spell toggle");
        BIND("meta-u",            "/window scroll_unread");
        BIND("meta-U",            "/allbuf /buffer set unread");
        BIND("ctrl-c,b",          "/input insert \\x02");
        BIND("ctrl-c,c",          "/input insert \\x03");
        BIND("ctrl-c,d",          "/input insert \\x04");
        BIND("ctrl-c,i",          "/input insert \\x1D");
        BIND("ctrl-c,o",          "/input insert \\x0F");
        BIND("ctrl-c,v",          "/input insert \\x16");
        BIND("ctrl-c,_",          "/input insert \\x1F");
        BIND("meta-right",        "/buffer +1");
        BIND("meta-down",         "/buffer +1");
        BIND("f6",                "/buffer +1");
        BIND("ctrl-n",            "/buffer +1");
        BIND("meta-left",         "/buffer -1");
        BIND("meta-up",           "/buffer -1");
        BIND("f5",                "/buffer -1");
        BIND("ctrl-p",            "/buffer -1");
        BIND("pgup",              "/window page_up");
        BIND("pgdn",              "/window page_down");
        BIND("meta-pgup",         "/window scroll_up");
        BIND("meta-pgdn",         "/window scroll_down");
        BIND("meta-home",         "/window scroll_top");
        BIND("meta-end",          "/window scroll_bottom");
        BIND("meta-n",            "/window scroll_next_highlight");
        BIND("meta-p",            "/window scroll_previous_highlight");
        BIND("meta-N",            "/bar toggle nicklist");
        BIND("f9",                "/bar scroll title * -30%");
        BIND("f10",               "/bar scroll title * +30%");
        BIND("f11",               "/bar scroll nicklist * -100%");
        BIND("f12",               "/bar scroll nicklist * +100%");
        BIND("ctrl-f11",          "/bar scroll nicklist * -100%");
        BIND("ctrl-f12",          "/bar scroll nicklist * +100%");
        BIND("meta-f11",          "/bar scroll nicklist * b");
        BIND("meta-f12",          "/bar scroll nicklist * e");
        BIND("ctrl-l",            "/window refresh");
        BIND("f7",                "/window -1");
        BIND("f8",                "/window +1");
        BIND("meta-w,meta-up",    "/window up");
        BIND("meta-w,meta-down",  "/window down");
        BIND("meta-w,meta-right", "/window right");
        BIND("meta-w,meta-left",  "/window left");
        BIND("meta-w,meta-b",     "/window balance");
        BIND("meta-w,meta-s",     "/window swap");
        BIND("meta-z",            "/window zoom");
        BIND("meta-=",            "/filter toggle");
        BIND("meta--",            "/filter toggle @");
        BIND("meta-0",            "/buffer *10");
        BIND("meta-1",            "/buffer *1");
        BIND("meta-2",            "/buffer *2");
        BIND("meta-3",            "/buffer *3");
        BIND("meta-4",            "/buffer *4");
        BIND("meta-5",            "/buffer *5");
        BIND("meta-6",            "/buffer *6");
        BIND("meta-7",            "/buffer *7");
        BIND("meta-8",            "/buffer *8");
        BIND("meta-9",            "/buffer *9");
        BIND("meta-<",            "/buffer jump prev_visited");
        BIND("meta->",            "/buffer jump next_visited");
        BIND("meta-/",            "/buffer jump last_displayed");
        BIND("meta-l",            "/window bare");
        BIND("meta-m",            "/mute mouse toggle");

        /* bind meta-j + {01..99} to switch to buffers # > 10 */
        for (i = 1; i < 100; i++)
        {
            snprintf (key_str, sizeof (key_str), "meta-j,%1d,%1d", i / 10, i % 10);
            snprintf (command, sizeof (command), "/buffer *%d", i);
            BIND(key_str, command);
        }
    }
    else if ((context == GUI_KEY_CONTEXT_SEARCH)
             || (context == GUI_KEY_CONTEXT_HISTSEARCH))
    {
        BIND("return", "/input search_stop_here");
        BIND("ctrl-q", "/input search_stop");
        BIND("meta-c", "/input search_switch_case");
        BIND("ctrl-x", "/input search_switch_regex");
        BIND("tab",    "/input search_switch_where");
        BIND("ctrl-r", "/input search_previous");
        BIND("up",     "/input search_previous");
        BIND("ctrl-s", "/input search_next");
        BIND("down",   "/input search_next");
        if (context == GUI_KEY_CONTEXT_HISTSEARCH)
        {
            BIND("ctrl-o", "/input history_use_get_next");
        }
    }
    else if (context == GUI_KEY_CONTEXT_CURSOR)
    {
        /* general & move */
        BIND("return",           "/cursor stop");
        BIND("up",               "/cursor move up");
        BIND("down",             "/cursor move down");
        BIND("left",             "/cursor move left");
        BIND("right",            "/cursor move right");
        BIND("meta-up",          "/cursor move edge_top");
        BIND("meta-down",        "/cursor move edge_bottom");
        BIND("meta-left",        "/cursor move edge_left");
        BIND("meta-right",       "/cursor move edge_right");
        BIND("meta-home",        "/cursor move top_left");
        BIND("meta-end",         "/cursor move bottom_right");
        BIND("meta-shift-up",    "/cursor move area_up");
        BIND("meta-shift-down",  "/cursor move area_down");
        BIND("meta-shift-left",  "/cursor move area_left");
        BIND("meta-shift-right", "/cursor move area_right");
        /* chat */
        BIND("@chat:m", "hsignal:chat_quote_message;/cursor stop");
        BIND("@chat:l", "hsignal:chat_quote_focused_line;/cursor stop");
        BIND("@chat:q", "hsignal:chat_quote_prefix_message;/cursor stop");
        BIND("@chat:Q", "hsignal:chat_quote_time_prefix_message;/cursor stop");
        /* nicklist */
        BIND("@item(buffer_nicklist):b", "/window ${_window_number};/ban ${nick}");
        BIND("@item(buffer_nicklist):k", "/window ${_window_number};/kick ${nick}");
        BIND("@item(buffer_nicklist):K", "/window ${_window_number};/kickban ${nick}");
        BIND("@item(buffer_nicklist):q", "/window ${_window_number};/query ${nick};/cursor stop");
        BIND("@item(buffer_nicklist):w", "/window ${_window_number};/whois ${nick}");
    }
    else if (context == GUI_KEY_CONTEXT_MOUSE)
    {
        /* mouse events on chat area */
        BIND("@chat:button1",                    "/window ${_window_number}");
        BIND("@chat:button1-gesture-left",       "/window ${_window_number};/buffer -1");
        BIND("@chat:button1-gesture-right",      "/window ${_window_number};/buffer +1");
        BIND("@chat:button1-gesture-left-long",  "/window ${_window_number};/buffer 1");
        BIND("@chat:button1-gesture-right-long", "/window ${_window_number};/buffer +");
        BIND("@chat:wheelup",                    "/window scroll_up -window ${_window_number}");
        BIND("@chat:wheeldown",                  "/window scroll_down -window ${_window_number}");
        BIND("@chat:ctrl-wheelup",               "/window scroll_horiz -window ${_window_number} -10%");
        BIND("@chat:ctrl-wheeldown",             "/window scroll_horiz -window ${_window_number} +10%");
        /* mouse events on nicklist */
        BIND("@bar(nicklist):button1-gesture-up",                "/bar scroll nicklist ${_window_number} -100%");
        BIND("@bar(nicklist):button1-gesture-down",              "/bar scroll nicklist ${_window_number} +100%");
        BIND("@bar(nicklist):button1-gesture-up-long",           "/bar scroll nicklist ${_window_number} b");
        BIND("@bar(nicklist):button1-gesture-down-long",         "/bar scroll nicklist ${_window_number} e");
        BIND("@item(buffer_nicklist):button1",                   "/window ${_window_number};/query ${nick}");
        BIND("@item(buffer_nicklist):button2",                   "/window ${_window_number};/whois ${nick}");
        BIND("@item(buffer_nicklist):button1-gesture-left",      "/window ${_window_number};/kick ${nick}");
        BIND("@item(buffer_nicklist):button1-gesture-left-long", "/window ${_window_number};/kickban ${nick}");
        BIND("@item(buffer_nicklist):button2-gesture-left",      "/window ${_window_number};/ban ${nick}");
        /* mouse events on input */
        BIND("@bar(input):button2", "/input grab_mouse_area");
        /* mouse wheel on any bar */
        BIND("@bar:wheelup",   "/bar scroll ${_bar_name} ${_window_number} -20%");
        BIND("@bar:wheeldown", "/bar scroll ${_bar_name} ${_window_number} +20%");
        /* middle click to enable cursor mode at position */
        BIND("@*:button3", "/cursor go ${_x},${_y}");
    }
}

/*
 * Flushes keyboard buffer.
 */

void
gui_key_flush (int paste)
{
    int i, key, last_key_used, insert_ok, undo_done;
    static char key_str[64] = { '\0' };
    static int length_key_str = 0;
    char key_temp[2], *key_utf, *input_old, *ptr_char, *next_char, *ptr_error;
    char utf_partial_char[16];
    struct t_gui_buffer *old_buffer;

    /* if paste pending or bracketed paste detected, just return */
    if (gui_key_paste_pending || gui_key_paste_bracketed)
        return;

    /* if buffer is empty, just return */
    if (gui_key_buffer_size == 0)
        return;

    /*
     * if there's no paste pending, then we use buffer and do actions
     * according to keys
     */
    gui_key_last_activity_time = time (NULL);
    last_key_used = -1;
    undo_done = 0;
    old_buffer = NULL;
    for (i = 0; i < gui_key_buffer_size; i++)
    {
        key = gui_key_buffer[i];

        /*
         * many terminal emulators send "\n" as "\r" when pasting, so replace
         * them back
         */
        if (paste && (key == '\r'))
            key = '\n';

        insert_ok = 1;
        utf_partial_char[0] = '\0';

        if (!paste && (key < 32))
        {
            insert_ok = 0;
            key_str[0] = '\x01';
            key_str[1] = (char)key + '@';
            /*
             * note: the terminal makes no difference between ctrl-x and
             * ctrl-shift-x, so for now WeeChat uses lower case letters for
             * ctrl keys
             */
            if ((key_str[1] >= 'A') && (key_str[1] <= 'Z'))
                key_str[1] += 'a' - 'A';
            key_str[2] = '\0';
            length_key_str = 2;
        }
        else if (!paste && gui_mouse_event_pending)
        {
            insert_ok = 0;
            key_str[0] = (char)key;
            key_str[1] = '\0';
            length_key_str = 1;
        }
        else if (!paste && (key == 127))
        {
            insert_ok = 0;
            key_str[0] = '\x01';
            key_str[1] = '?';
            key_str[2] = '\0';
            length_key_str = 2;
        }
        else if (local_utf8)
        {
            key_str[length_key_str] = (char)key;
            key_str[length_key_str + 1] = '\0';
            length_key_str++;

            /*
             * replace invalid chars by "?", but NOT last char of
             * string, if it is incomplete UTF-8 char (another char
             * will be added to the string on next iteration)
             */
            ptr_char = key_str;
            while (ptr_char && ptr_char[0])
            {
                (void) utf8_is_valid (ptr_char, -1, &ptr_error);
                if (!ptr_error)
                    break;
                next_char = (char *)utf8_next_char (ptr_error);
                if (next_char && next_char[0])
                {
                    ptr_char = ptr_error;
                    while (ptr_char < next_char)
                    {
                        ptr_char[0] = '?';
                        ptr_char++;
                    }
                }
                else
                {
                    strcpy (utf_partial_char, ptr_char);
                    ptr_char[0] = '\0';
                    break;
                }
                ptr_char = next_char;
            }
        }
        else
        {
            /* convert input to UTF-8 */
            key_temp[0] = (char)key;
            key_temp[1] = '\0';
            key_utf = string_iconv_to_internal (NULL, key_temp);
            strcat (key_str, key_utf);
        }

        if (key_str[0])
        {
            /*
             * send the signal "key_pressed" only if NOT reading a mouse event
             * or if the mouse code is valid UTF-8 (do not send partial mouse
             * code which is not UTF-8 valid)
             */
            if (!paste
                && (i > gui_key_last_key_pressed_sent)
                && (!gui_mouse_event_pending
                    || utf8_is_valid (key_str, -1, NULL)))
            {
                (void) hook_signal_send ("key_pressed",
                                         WEECHAT_HOOK_SIGNAL_STRING, key_str);
                gui_key_last_key_pressed_sent = i;
            }

            if (gui_current_window->buffer->text_search != GUI_BUFFER_SEARCH_DISABLED)
                input_old = (gui_current_window->buffer->input_buffer) ?
                    strdup (gui_current_window->buffer->input_buffer) : strdup ("");
            else
                input_old = NULL;
            old_buffer = gui_current_window->buffer;

            if ((paste || gui_key_pressed (key_str) != 0)
                && insert_ok
                && !gui_cursor_mode)
            {
                if (!paste || !undo_done)
                    gui_buffer_undo_snap (gui_current_window->buffer);
                gui_input_insert_string (gui_current_window->buffer, key_str);
                gui_input_text_changed_modifier_and_signal (gui_current_window->buffer,
                                                            (!paste || !undo_done) ? 1 : 0,
                                                            1); /* stop completion */
                undo_done = 1;
            }

            /* incremental text search in buffer lines or command line history */
            if ((old_buffer == gui_current_window->buffer)
                && ((gui_current_window->buffer->text_search == GUI_BUFFER_SEARCH_LINES)
                    || (gui_current_window->buffer->text_search == GUI_BUFFER_SEARCH_HISTORY))
                && ((input_old == NULL)
                    || (gui_current_window->buffer->input_buffer == NULL)
                    || (strcmp (input_old, gui_current_window->buffer->input_buffer) != 0)))
            {
                /*
                 * if following conditions are all true, then do not search
                 * again (search will not find any result and can take some time):
                 * - old search was not successful
                 * - searching a string (not a regex)
                 * - current input is longer than old input
                 * - beginning of current input is exactly equal to old input.
                 */
                if (!gui_current_window->buffer->text_search_found
                    && !gui_current_window->buffer->text_search_regex
                    && (input_old != NULL)
                    && (input_old[0])
                    && (gui_current_window->buffer->input_buffer != NULL)
                    && (gui_current_window->buffer->input_buffer[0])
                    && (strlen (gui_current_window->buffer->input_buffer) > strlen (input_old))
                    && (strncmp (gui_current_window->buffer->input_buffer, input_old,
                                 strlen (input_old)) == 0))
                {
                    /* do not search text, just alert about text not found */
                    if (CONFIG_BOOLEAN(config_look_search_text_not_found_alert))
                    {
                        fprintf (stderr, "\a");
                        fflush (stderr);
                    }
                }
                else
                {
                    gui_window_search_restart (gui_current_window);
                }
            }

            free (input_old);
        }

        /* prepare incomplete UTF-8 char for next iteration */
        if (utf_partial_char[0])
            strcpy (key_str, utf_partial_char);
        else
            key_str[0] = '\0';
        length_key_str = strlen (key_str);

        /* set last key used in buffer if combo buffer is empty */
        if (gui_key_grab || gui_mouse_event_pending || !gui_key_combo[0])
            last_key_used = i;
    }

    if (last_key_used == gui_key_buffer_size - 1)
        gui_key_buffer_reset ();
    else if (last_key_used >= 0)
        gui_key_buffer_remove (0, last_key_used + 1);

    if (!gui_key_grab && !gui_mouse_event_pending)
        gui_key_combo[0] = '\0';
}

/*
 * Reads keyboard chars.
 */

int
gui_key_read_cb (const void *pointer, void *data, int fd)
{
    int ret, i, accept_paste, cancel_paste, text_added_to_buffer, pos;
    unsigned char buffer[4096];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) fd;

    accept_paste = 0;
    cancel_paste = 0;
    text_added_to_buffer = 0;

    ret = read (STDIN_FILENO, buffer, sizeof (buffer));
    if (ret == 0)
    {
        /* no data on stdin, terminal lost */
        if (!weechat_quit)
        {
            log_printf (_("Terminal lost, exiting WeeChat..."));
            (void) hook_signal_send ("quit", WEECHAT_HOOK_SIGNAL_STRING, NULL);
            weechat_quit = 1;
        }
        return WEECHAT_RC_OK;
    }
    if (ret < 0)
        return WEECHAT_RC_OK;

    for (i = 0; i < ret; i++)
    {
        if (gui_key_paste_pending && (buffer[i] == 25))
        {
            /* ctrl-y: accept paste */
            accept_paste = 1;
        }
        else if (gui_key_paste_pending && (buffer[i] == 14))
        {
            /* ctrl-n: cancel paste */
            cancel_paste = 1;
        }
        else
        {
            gui_key_buffer_add (buffer[i]);
            text_added_to_buffer = 1;
        }
    }

    if (!gui_key_paste_bracketed)
    {
        pos = gui_key_buffer_search (0, -1, GUI_KEY_BRACKETED_PASTE_START);
        if (pos >= 0)
        {
            gui_key_buffer_remove (pos, GUI_KEY_BRACKETED_PASTE_LENGTH);
            gui_key_paste_bracketed_start ();
        }
    }

    if (!gui_key_paste_bracketed)
    {
        if (gui_key_paste_pending)
        {
            if (accept_paste)
            {
                /* user is OK for pasting text, let's paste! */
                gui_key_paste_accept ();
            }
            else if (cancel_paste)
            {
                /* user doesn't want to paste text: clear whole buffer! */
                gui_key_paste_cancel ();
            }
            else if (text_added_to_buffer)
            {
                /* new text received while asking for paste, update message */
                gui_input_paste_pending_signal ();
            }
        }
        else
            gui_key_paste_check (0);
    }

    gui_key_flush ((accept_paste) ? 1 : 0);

    if (gui_key_paste_bracketed)
    {
        pos = gui_key_buffer_search (0, -1, GUI_KEY_BRACKETED_PASTE_END);
        if (pos >= 0)
        {
            /* remove the code for end of bracketed paste (ESC[201~) */
            gui_key_buffer_remove (pos, GUI_KEY_BRACKETED_PASTE_LENGTH);

            /* stop bracketed mode */
            gui_key_paste_bracketed_timer_remove ();
            gui_key_paste_bracketed_stop ();

            /* if paste confirmation not displayed, flush buffer now */
            if (!gui_key_paste_pending)
            {
                gui_key_paste_finish ();
                gui_key_flush (1);
            }
        }
    }

    return WEECHAT_RC_OK;
}
