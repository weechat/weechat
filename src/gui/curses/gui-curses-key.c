/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * gui-curses-key.c: keyboard functions for Curses GUI
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-hook.h"
#include "../../core/wee-log.h"
#include "../../core/wee-utf8.h"
#include "../../core/wee-string.h"
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

#define BIND(key, command) gui_key_default_bind(context, key, command)


/*
 * gui_key_default_bind: create key bind, only if it does not exist yet
 */

void
gui_key_default_bind (int context, const char *key, const char *command)
{
    struct t_gui_key *ptr_key;
    char *internal_code;

    internal_code = gui_key_get_internal_code (key);

    ptr_key = gui_key_search (gui_keys[context],
                              (internal_code) ? internal_code : key);
    if (!ptr_key)
        gui_key_new (NULL, context, key, command);

    if (internal_code)
        free (internal_code);
}

/*
 * gui_key_default_bindings: create default key bindings for context given
 */

void
gui_key_default_bindings (int context)
{
    int i;
    char key_str[32], command[32];

    if (context == GUI_KEY_CONTEXT_DEFAULT)
    {
        BIND(/* Enter       */ "ctrl-M",             "/input return");
        BIND(/* Enter       */ "ctrl-J",             "/input return");
        BIND(/* tab         */ "ctrl-I",             "/input complete_next");
        BIND(/* s-tab       */ "meta2-Z",            "/input complete_previous");
        BIND(/* ^R          */ "ctrl-R",             "/input search_text");
        BIND(/* backspace   */ "ctrl-H",             "/input delete_previous_char");
        BIND(/* backspace   */ "ctrl-?",             "/input delete_previous_char");
        BIND(/* ^_          */ "ctrl-_",             "/input undo");
        BIND(/* m-_         */ "meta-_",             "/input redo");
        BIND(/* del         */ "meta2-3~",           "/input delete_next_char");
        BIND(/* ^D          */ "ctrl-D",             "/input delete_next_char");
        BIND(/* ^W          */ "ctrl-W",             "/input delete_previous_word");
        BIND(/* ^X          */ "ctrl-X",             "/input switch_active_buffer");
        BIND(/* m-d         */ "meta-d",             "/input delete_next_word");
        BIND(/* ^K          */ "ctrl-K",             "/input delete_end_of_line");
        BIND(/* m-r         */ "meta-r",             "/input delete_line");
        BIND(/* ^T          */ "ctrl-T",             "/input transpose_chars");
        BIND(/* ^U          */ "ctrl-U",             "/input delete_beginning_of_line");
        BIND(/* ^Y          */ "ctrl-Y",             "/input clipboard_paste");
        BIND(/* home        */ "meta2-1~",           "/input move_beginning_of_line");
        BIND(/* home        */ "meta2-H",            "/input move_beginning_of_line");
        BIND(/* home        */ "meta2-7~",           "/input move_beginning_of_line");
        BIND(/* home        */ "meta-OH",            "/input move_beginning_of_line");
        BIND(/* ^A          */ "ctrl-A",             "/input move_beginning_of_line");
        BIND(/* end         */ "meta2-4~",           "/input move_end_of_line");
        BIND(/* end         */ "meta2-F",            "/input move_end_of_line");
        BIND(/* end         */ "meta2-8~",           "/input move_end_of_line");
        BIND(/* end         */ "meta-OF",            "/input move_end_of_line");
        BIND(/* ^E          */ "ctrl-E",             "/input move_end_of_line");
        BIND(/* left        */ "meta2-D",            "/input move_previous_char");
        BIND(/* ^B          */ "ctrl-B",             "/input move_previous_char");
        BIND(/* right       */ "meta2-C",            "/input move_next_char");
        BIND(/* ^F          */ "ctrl-F",             "/input move_next_char");
        BIND(/* m-b         */ "meta-b",             "/input move_previous_word");
        BIND(/* ^left       */ "meta-Od",            "/input move_previous_word");
        BIND(/* ^left       */ "meta-OD",            "/input move_previous_word");
        BIND(/* ^left       */ "meta2-1;5D",         "/input move_previous_word");
        BIND(/* m-f         */ "meta-f",             "/input move_next_word");
        BIND(/* ^right      */ "meta-Oc",            "/input move_next_word");
        BIND(/* ^right      */ "meta-OC",            "/input move_next_word");
        BIND(/* ^right      */ "meta2-1;5C",         "/input move_next_word");
        BIND(/* up          */ "meta2-A",            "/input history_previous");
        BIND(/* down        */ "meta2-B",            "/input history_next");
        BIND(/* ^up         */ "meta-Oa",            "/input history_global_previous");
        BIND(/* ^up         */ "meta-OA",            "/input history_global_previous");
        BIND(/* ^up         */ "meta2-1;5A",         "/input history_global_previous");
        BIND(/* ^down       */ "meta-Ob",            "/input history_global_next");
        BIND(/* ^down       */ "meta-OB",            "/input history_global_next");
        BIND(/* ^down       */ "meta2-1;5B",         "/input history_global_next");
        BIND(/* m-a         */ "meta-a",             "/input jump_smart");
        BIND(/* m-j,m-l     */ "meta-jmeta-l",       "/input jump_last_buffer");
        BIND(/* m-j,m-r     */ "meta-jmeta-r",       "/server raw");
        BIND(/* m-j,m-s     */ "meta-jmeta-s",       "/server jump");
        BIND(/* m-h         */ "meta-h",             "/input hotlist_clear");
        BIND(/* m-k         */ "meta-k",             "/input grab_key_command");
        BIND(/* m-u         */ "meta-u",             "/window scroll_unread");
        BIND(/* ^S^U        */ "ctrl-Sctrl-U",       "/input set_unread");
        BIND(/* ^Cb         */ "ctrl-Cb",            "/input insert \\x02");
        BIND(/* ^Cc         */ "ctrl-Cc",            "/input insert \\x03");
        BIND(/* ^Ci         */ "ctrl-Ci",            "/input insert \\x1D");
        BIND(/* ^Co         */ "ctrl-Co",            "/input insert \\x0F");
        BIND(/* ^Cr         */ "ctrl-Cr",            "/input insert \\x12");
        BIND(/* ^Cu         */ "ctrl-Cu",            "/input insert \\x15");
        BIND(/* m-right     */ "meta-meta2-C",       "/buffer +1");
        BIND(/* m-right     */ "meta2-1;3C",         "/buffer +1");
        BIND(/* m-down      */ "meta-meta2-B",       "/buffer +1");
        BIND(/* m-down      */ "meta2-1;3B",         "/buffer +1");
        BIND(/* F6          */ "meta2-17~",          "/buffer +1");
        BIND(/* ^N          */ "ctrl-N",             "/buffer +1");
        BIND(/* m-left      */ "meta-meta2-D",       "/buffer -1");
        BIND(/* m-left      */ "meta2-1;3D",         "/buffer -1");
        BIND(/* m-up        */ "meta-meta2-A",       "/buffer -1");
        BIND(/* m-up        */ "meta2-1;3A",         "/buffer -1");
        BIND(/* F5          */ "meta2-15~",          "/buffer -1");
        BIND(/* F5          */ "meta2-[E",           "/buffer -1");
        BIND(/* ^P          */ "ctrl-P",             "/buffer -1");
        BIND(/* pgup        */ "meta2-5~",           "/window page_up");
        BIND(/* pgup        */ "meta2-I",            "/window page_up");
        BIND(/* pgdn        */ "meta2-6~",           "/window page_down");
        BIND(/* pgdn        */ "meta2-G",            "/window page_down");
        BIND(/* m-pgup      */ "meta-meta2-5~",      "/window scroll_up");
        BIND(/* m-pgup      */ "meta2-5;3~",         "/window scroll_up");
        BIND(/* m-pgdn      */ "meta-meta2-6~",      "/window scroll_down");
        BIND(/* m-pgdn      */ "meta2-6;3~",         "/window scroll_down");
        BIND(/* m-home      */ "meta-meta2-1~",      "/window scroll_top");
        BIND(/* m-home      */ "meta-meta2-7~",      "/window scroll_top");
        BIND(/* m-end       */ "meta-meta2-4~",      "/window scroll_bottom");
        BIND(/* m-end       */ "meta-meta2-8~",      "/window scroll_bottom");
        BIND(/* m-n         */ "meta-n",             "/window scroll_next_highlight");
        BIND(/* m-p         */ "meta-p",             "/window scroll_previous_highlight");
        BIND(/* F9          */ "meta2-20~",          "/bar scroll title * -30%");
        BIND(/* F10         */ "meta2-21~",          "/bar scroll title * +30%");
        BIND(/* F11         */ "meta2-23~",          "/bar scroll nicklist * -100%");
        BIND(/* F12         */ "meta2-24~",          "/bar scroll nicklist * +100%");
        BIND(/* m-F11       */ "meta-meta2-23~",     "/bar scroll nicklist * b");
        BIND(/* m-F12       */ "meta-meta2-24~",     "/bar scroll nicklist * e");
        BIND(/* ^L          */ "ctrl-L",             "/window refresh");
        BIND(/* F7          */ "meta2-18~",          "/window -1");
        BIND(/* F8          */ "meta2-19~",          "/window +1");
        BIND(/* m-w,m-up    */ "meta-wmeta-meta2-A", "/window up");
        BIND(/* m-w,m-up    */ "meta-wmeta2-1;3A",   "/window up");
        BIND(/* m-w,m-down  */ "meta-wmeta-meta2-B", "/window down");
        BIND(/* m-w,m-down  */ "meta-wmeta2-1;3B",   "/window down");
        BIND(/* m-w,m-right */ "meta-wmeta-meta2-C", "/window right");
        BIND(/* m-w,m-right */ "meta-wmeta2-1;3C",   "/window right");
        BIND(/* m-w,m-left  */ "meta-wmeta-meta2-D", "/window left");
        BIND(/* m-w,m-left  */ "meta-wmeta2-1;3D",   "/window left");
        BIND(/* m-w,m-b     */ "meta-wmeta-b",       "/window balance");
        BIND(/* m-w,m-s     */ "meta-wmeta-s",       "/window swap");
        BIND(/* m-z         */ "meta-z",             "/window zoom");
        BIND(/* m-=         */ "meta-=",             "/filter toggle");
        BIND(/* m-0         */ "meta-0",             "/buffer *10");
        BIND(/* m-1         */ "meta-1",             "/buffer *1");
        BIND(/* m-2         */ "meta-2",             "/buffer *2");
        BIND(/* m-3         */ "meta-3",             "/buffer *3");
        BIND(/* m-4         */ "meta-4",             "/buffer *4");
        BIND(/* m-5         */ "meta-5",             "/buffer *5");
        BIND(/* m-6         */ "meta-6",             "/buffer *6");
        BIND(/* m-7         */ "meta-7",             "/buffer *7");
        BIND(/* m-8         */ "meta-8",             "/buffer *8");
        BIND(/* m-9         */ "meta-9",             "/buffer *9");
        BIND(/* m-<         */ "meta-<",             "/input jump_previously_visited_buffer");
        BIND(/* m->         */ "meta->",             "/input jump_next_visited_buffer");
        BIND(/* m-/         */ "meta-/",             "/input jump_last_buffer_displayed");
        BIND(/* m-m         */ "meta-m",             "/mute mouse toggle");
        BIND(/* start paste */ "meta2-200~",         "/input paste_start");
        BIND(/* end paste   */ "meta2-201~",         "/input paste_stop");

        /* bind meta-j + {01..99} to switch to buffers # > 10 */
        for (i = 1; i < 100; i++)
        {
            sprintf (key_str, "meta-j%02d", i);
            sprintf (command, "/buffer %d", i);
            BIND(key_str, command);
        }
    }
    else if (context == GUI_KEY_CONTEXT_SEARCH)
    {
        BIND(/* Enter */ "ctrl-M",  "/input search_stop");
        BIND(/* Enter */ "ctrl-J",  "/input search_stop");
        BIND(/* ^R    */ "ctrl-R",  "/input search_switch_case");
        BIND(/* up    */ "meta2-A", "/input search_previous");
        BIND(/* down  */ "meta2-B", "/input search_next");
    }
    else if (context == GUI_KEY_CONTEXT_CURSOR)
    {
        /* general & move */
        BIND(/* Enter   */ "ctrl-M",                   "/cursor stop");
        BIND(/* Enter   */ "ctrl-J",                   "/cursor stop");
        BIND(/* up      */ "meta2-A",                  "/cursor move up");
        BIND(/* down    */ "meta2-B",                  "/cursor move down");
        BIND(/* left    */ "meta2-D",                  "/cursor move left");
        BIND(/* right   */ "meta2-C",                  "/cursor move right");
        BIND(/* m-up    */ "meta-meta2-A",             "/cursor move area_up");
        BIND(/* m-up    */ "meta2-1;3A",               "/cursor move area_up");
        BIND(/* m-down  */ "meta-meta2-B",             "/cursor move area_down");
        BIND(/* m-down  */ "meta2-1;3B",               "/cursor move area_down");
        BIND(/* m-left  */ "meta-meta2-D",             "/cursor move area_left");
        BIND(/* m-left  */ "meta2-1;3D",               "/cursor move area_left");
        BIND(/* m-right */ "meta-meta2-C",             "/cursor move area_right");
        BIND(/* m-right */ "meta2-1;3C",               "/cursor move area_right");
        /* chat */
        BIND(/* m       */ "@chat:m",                  "hsignal:chat_quote_message;/cursor stop");
        BIND(/* q       */ "@chat:q",                  "hsignal:chat_quote_prefix_message;/cursor stop");
        BIND(/* Q       */ "@chat:Q",                  "hsignal:chat_quote_time_prefix_message;/cursor stop");
        /* nicklist */
        BIND(/* b       */ "@item(buffer_nicklist):b", "/window ${_window_number};/ban ${nick}");
        BIND(/* k       */ "@item(buffer_nicklist):k", "/window ${_window_number};/kick ${nick}");
        BIND(/* K       */ "@item(buffer_nicklist):K", "/window ${_window_number};/kickban ${nick}");
        BIND(/* q       */ "@item(buffer_nicklist):q", "/window ${_window_number};/query ${nick};/cursor stop");
        BIND(/* w       */ "@item(buffer_nicklist):w", "/window ${_window_number};/whois ${nick}");
    }
    else if (context == GUI_KEY_CONTEXT_MOUSE)
    {
        /* mouse events on chat area */
        BIND("@chat:button1",                    "/window ${_window_number}");
        BIND("@chat:button1-gesture-left",       "/window ${_window_number};/buffer -1");
        BIND("@chat:button1-gesture-right",      "/window ${_window_number};/buffer +1");
        BIND("@chat:button1-gesture-left-long",  "/window ${_window_number};/buffer 1");
        BIND("@chat:button1-gesture-right-long", "/window ${_window_number};/input jump_last_buffer");
        BIND("@chat:wheelup",                    "/window scroll_up -window ${_window_number}");
        BIND("@chat:wheeldown",                  "/window scroll_down -window ${_window_number}");
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
 * gui_key_flush: flush keyboard buffer
 */

void
gui_key_flush (int paste)
{
    int i, key, last_key_used, insert_ok, undo_done;
    static char key_str[64] = { '\0' };
    static int length_key_str = 0;
    char key_temp[2], *key_utf, *input_old, *ptr_char, *next_char, *ptr_error;
    char utf_partial_char[16];

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
    for (i = 0; i < gui_key_buffer_size; i++)
    {
        key = gui_key_buffer[i];
        insert_ok = 1;
        utf_partial_char[0] = '\0';

        if (gui_mouse_event_pending || (key < 32) || (key == 127))
        {
            if (gui_mouse_event_pending)
            {
                insert_ok = 0;
                key_str[0] = (char)key;
                key_str[1] = '\0';
                length_key_str = 1;
            }
            else if (key < 32)
            {
                insert_ok = 0;
                key_str[0] = '\x01';
                key_str[1] = (char)key + '@';
                key_str[2] = '\0';
                length_key_str = 2;
            }
            else if (key == 127)
            {
                key_str[0] = '\x01';
                key_str[1] = '?';
                key_str[2] = '\0';
                length_key_str = 2;
            }
        }
        else
        {
            if (local_utf8)
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
                    (void) utf8_is_valid (ptr_char, &ptr_error);
                    if (!ptr_error)
                        break;
                    next_char = utf8_next_char (ptr_error);
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
        }

        if (key_str[0])
        {
            hook_signal_send ("key_pressed",
                              WEECHAT_HOOK_SIGNAL_STRING, key_str);

            if (gui_current_window->buffer->text_search != GUI_TEXT_SEARCH_DISABLED)
                input_old = (gui_current_window->buffer->input_buffer) ?
                    strdup (gui_current_window->buffer->input_buffer) : strdup ("");
            else
                input_old = NULL;

            if ((gui_key_pressed (key_str) != 0) && (insert_ok)
                && (!gui_cursor_mode))
            {
                if (!paste || !undo_done)
                    gui_buffer_undo_snap (gui_current_window->buffer);
                gui_input_insert_string (gui_current_window->buffer,
                                         key_str, -1);
                if (gui_current_window->buffer->completion)
                    gui_completion_stop (gui_current_window->buffer->completion, 0);
                gui_input_text_changed_modifier_and_signal (gui_current_window->buffer,
                                                            (!paste || !undo_done) ? 1 : 0);
                undo_done = 1;
            }

            /* incremental text search in buffer */
            if ((gui_current_window->buffer->text_search != GUI_TEXT_SEARCH_DISABLED)
                && ((input_old == NULL)
                    || (gui_current_window->buffer->input_buffer == NULL)
                    || (strcmp (input_old, gui_current_window->buffer->input_buffer) != 0)))
            {
                /*
                 * if current input is longer than old input, and that
                 * beginning of current input is exactly equal to old input,
                 * then do nothing (search will not find any result and can
                 * take some time on buffer with many lines..)
                 */
                if (!gui_current_window->buffer->text_search_found
                    && (input_old != NULL)
                    && (input_old[0])
                    && (gui_current_window->buffer->input_buffer != NULL)
                    && (gui_current_window->buffer->input_buffer[0])
                    && (strlen (gui_current_window->buffer->input_buffer) > strlen (input_old))
                    && (strncmp (gui_current_window->buffer->input_buffer, input_old,
                                 strlen (input_old)) == 0))
                {
                    /*
                     * do not search text in buffer, just alert about text not
                     * found
                     */
                    if (CONFIG_BOOLEAN(config_look_search_text_not_found_alert))
                        printf ("\a");
                }
                else
                {
                    gui_window_search_restart (gui_current_window);
                }
            }

            if (input_old)
                free (input_old);
        }

        /* prepare incomplete UTF-8 char for next iteration */
        if (utf_partial_char[0])
            strcpy (key_str, utf_partial_char);
        else
            key_str[0] = '\0';
        length_key_str = strlen (key_str);

        /* set last key used in buffer if combo buffer is empty */
        if (gui_mouse_event_pending || !gui_key_combo_buffer[0])
            last_key_used = i;
    }

    if (last_key_used == gui_key_buffer_size - 1)
        gui_key_buffer_reset ();
    else if (last_key_used >= 0)
        gui_key_buffer_remove (0, last_key_used + 1);

    if (!gui_mouse_event_pending)
        gui_key_combo_buffer[0] = '\0';
}

/*
 * gui_key_read_cb: read keyboard chars
 */

int
gui_key_read_cb (void *data, int fd)
{
    int ret, i, accept_paste, cancel_paste, text_added_to_buffer, pos;
    unsigned char buffer[4096];

    /* make C compiler happy */
    (void) data;
    (void) fd;

    accept_paste = 0;
    cancel_paste = 0;
    text_added_to_buffer = 0;

    ret = read (STDIN_FILENO, buffer, sizeof (buffer));
    if (ret == 0)
    {
        /* no data on stdin, terminal lost */
        log_printf (_("Terminal lost, exiting WeeChat..."));
        hook_signal_send ("quit", WEECHAT_HOOK_SIGNAL_STRING, NULL);
        weechat_quit = 1;
        return WEECHAT_RC_OK;
    }
    if (ret < 0)
        return WEECHAT_RC_OK;

    for (i = 0; i < ret; i++)
    {
        /*
         * add all chars, but ignore a newline ('\r' or '\n') after
         * another one)
         */
        if ((i == 0)
            || ((buffer[i] != '\r') && (buffer[i] != '\n'))
            || ((buffer[i - 1] != '\r') && (buffer[i - 1] != '\n')))
        {
            if (gui_key_paste_pending)
            {
                if (buffer[i] == 25)
                {
                    /* ctrl-Y: accept paste */
                    accept_paste = 1;
                }
                else if (buffer[i] == 14)
                {
                    /* ctrl-N: cancel paste */
                    cancel_paste = 1;
                }
            }
            else
            {
                gui_key_buffer_add (buffer[i]);
                text_added_to_buffer = 1;
            }
        }
    }

    if (gui_key_paste_pending)
    {
        if (accept_paste)
        {
            /* user is ok for pasting text, let's paste! */
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
    {
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

            /* remove final newline (if needed) */
            gui_key_paste_remove_newline ();

            /* stop bracketed mode */
            gui_key_paste_bracketed_timer_remove ();
            gui_key_paste_bracketed_stop ();

            /* if paste confirmation not displayed, flush buffer now */
            if (!gui_key_paste_pending)
                gui_key_flush (1);
        }
    }

    return WEECHAT_RC_OK;
}
