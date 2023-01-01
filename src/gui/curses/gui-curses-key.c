/*
 * gui-curses-key.c - keyboard functions for Curses GUI
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
 * Creates key bind, only if it does not exist yet.
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
 * Creates default key bindings for a given context.
 */

void
gui_key_default_bindings (int context)
{
    int i;
    char key_str[32], command[32];

    if (context == GUI_KEY_CONTEXT_DEFAULT)
    {
        BIND(/* <enter>       */ "ctrl-M",             "/input return");
        BIND(/* <enter>       */ "ctrl-J",             "/input return");
        BIND(/* m-<enter>     */ "meta-ctrl-M",        "/input insert \\n");
        BIND(/* <tab>         */ "ctrl-I",             "/input complete_next");
        BIND(/* s-<tab>       */ "meta2-Z",            "/input complete_previous");
        BIND(/* ^R            */ "ctrl-R",             "/input search_text_here");
        BIND(/* <backspace>   */ "ctrl-H",             "/input delete_previous_char");
        BIND(/* <backspace>   */ "ctrl-?",             "/input delete_previous_char");
        BIND(/* ^_            */ "ctrl-_",             "/input undo");
        BIND(/* m-_           */ "meta-_",             "/input redo");
        BIND(/* <del>         */ "meta2-3~",           "/input delete_next_char");
        BIND(/* ^D            */ "ctrl-D",             "/input delete_next_char");
        BIND(/* ^W            */ "ctrl-W",             "/input delete_previous_word_whitespace");
        BIND(/* m-<backspace> */ "meta-ctrl-?",        "/input delete_previous_word");
        BIND(/* ^X            */ "ctrl-X",             "/buffer switch");
        BIND(/* m-x           */ "meta-x",             "/buffer zoom");
        BIND(/* m-d           */ "meta-d",             "/input delete_next_word");
        BIND(/* ^K            */ "ctrl-K",             "/input delete_end_of_line");
        BIND(/* m-r           */ "meta-r",             "/input delete_line");
        BIND(/* ^T            */ "ctrl-T",             "/input transpose_chars");
        BIND(/* ^U            */ "ctrl-U",             "/input delete_beginning_of_line");
        BIND(/* ^Y            */ "ctrl-Y",             "/input clipboard_paste");
        BIND(/* <home>        */ "meta2-1~",           "/input move_beginning_of_line");
        BIND(/* <home>        */ "meta2-H",            "/input move_beginning_of_line");
        BIND(/* <home>        */ "meta2-7~",           "/input move_beginning_of_line");
        BIND(/* <home>        */ "meta-OH",            "/input move_beginning_of_line");
        BIND(/* ^A            */ "ctrl-A",             "/input move_beginning_of_line");
        BIND(/* <end>         */ "meta2-4~",           "/input move_end_of_line");
        BIND(/* <end>         */ "meta2-F",            "/input move_end_of_line");
        BIND(/* <end>         */ "meta2-8~",           "/input move_end_of_line");
        BIND(/* <end>         */ "meta-OF",            "/input move_end_of_line");
        BIND(/* ^E            */ "ctrl-E",             "/input move_end_of_line");
        BIND(/* <left>        */ "meta2-D",            "/input move_previous_char");
        BIND(/* ^B            */ "ctrl-B",             "/input move_previous_char");
        BIND(/* <right>       */ "meta2-C",            "/input move_next_char");
        BIND(/* ^F            */ "ctrl-F",             "/input move_next_char");
        BIND(/* m-b           */ "meta-b",             "/input move_previous_word");
        BIND(/* ^<left>       */ "meta-Od",            "/input move_previous_word");
        BIND(/* ^<left>       */ "meta-OD",            "/input move_previous_word");
        BIND(/* ^<left>       */ "meta2-1;5D",         "/input move_previous_word");
        BIND(/* m-f           */ "meta-f",             "/input move_next_word");
        BIND(/* ^<right>      */ "meta-Oc",            "/input move_next_word");
        BIND(/* ^<right>      */ "meta-OC",            "/input move_next_word");
        BIND(/* ^<right>      */ "meta2-1;5C",         "/input move_next_word");
        BIND(/* <up>          */ "meta2-A",            "/input history_previous");
        BIND(/* <down>        */ "meta2-B",            "/input history_next");
        BIND(/* ^<up>         */ "meta-Oa",            "/input history_global_previous");
        BIND(/* ^<up>         */ "meta-OA",            "/input history_global_previous");
        BIND(/* ^<up>         */ "meta2-1;5A",         "/input history_global_previous");
        BIND(/* ^<down>       */ "meta-Ob",            "/input history_global_next");
        BIND(/* ^<down>       */ "meta-OB",            "/input history_global_next");
        BIND(/* ^<down>       */ "meta2-1;5B",         "/input history_global_next");
        BIND(/* m-a           */ "meta-a",             "/buffer jump smart");
        BIND(/* m-j,m-f       */ "meta-jmeta-f",       "/buffer -");
        BIND(/* m-j,m-l       */ "meta-jmeta-l",       "/buffer +");
        BIND(/* m-j,m-r       */ "meta-jmeta-r",       "/server raw");
        BIND(/* m-j,m-s       */ "meta-jmeta-s",       "/server jump");
        BIND(/* m-h,m-c       */ "meta-hmeta-c",       "/hotlist clear");
        BIND(/* m-h,m-m       */ "meta-hmeta-m",       "/hotlist remove");
        BIND(/* m-h,m-r       */ "meta-hmeta-r",       "/hotlist restore");
        BIND(/* m-h,m-R       */ "meta-hmeta-R",       "/hotlist restore -all");
        BIND(/* m-k           */ "meta-k",             "/input grab_key_command");
        BIND(/* m-s           */ "meta-s",             "/mute spell toggle");
        BIND(/* m-u           */ "meta-u",             "/window scroll_unread");
        BIND(/* ^S^U          */ "ctrl-Sctrl-U",       "/allbuf /buffer set unread");
        BIND(/* ^Cb           */ "ctrl-Cb",            "/input insert \\x02");
        BIND(/* ^Cc           */ "ctrl-Cc",            "/input insert \\x03");
        BIND(/* ^Ci           */ "ctrl-Ci",            "/input insert \\x1D");
        BIND(/* ^Co           */ "ctrl-Co",            "/input insert \\x0F");
        BIND(/* ^Cv           */ "ctrl-Cv",            "/input insert \\x16");
        BIND(/* ^C_           */ "ctrl-C_",            "/input insert \\x1F");
        BIND(/* m-<right>     */ "meta-meta2-C",       "/buffer +1");
        BIND(/* m-<right>     */ "meta2-1;3C",         "/buffer +1");
        BIND(/* m-<down>      */ "meta-meta2-B",       "/buffer +1");
        BIND(/* m-<down>      */ "meta2-1;3B",         "/buffer +1");
        BIND(/* <f6>          */ "meta2-17~",          "/buffer +1");
        BIND(/* ^N            */ "ctrl-N",             "/buffer +1");
        BIND(/* m-<left>      */ "meta-meta2-D",       "/buffer -1");
        BIND(/* m-<left>      */ "meta2-1;3D",         "/buffer -1");
        BIND(/* m-<up>        */ "meta-meta2-A",       "/buffer -1");
        BIND(/* m-<up>        */ "meta2-1;3A",         "/buffer -1");
        BIND(/* <f5>          */ "meta2-15~",          "/buffer -1");
        BIND(/* <f5>          */ "meta2-[E",           "/buffer -1");
        BIND(/* ^P            */ "ctrl-P",             "/buffer -1");
        BIND(/* <pgup>        */ "meta2-5~",           "/window page_up");
        BIND(/* <pgup>        */ "meta2-I",            "/window page_up");
        BIND(/* <pgdn>        */ "meta2-6~",           "/window page_down");
        BIND(/* <pgdn>        */ "meta2-G",            "/window page_down");
        BIND(/* m-<pgup>      */ "meta-meta2-5~",      "/window scroll_up");
        BIND(/* m-<pgup>      */ "meta2-5;3~",         "/window scroll_up");
        BIND(/* m-<pgdn>      */ "meta-meta2-6~",      "/window scroll_down");
        BIND(/* m-<pgdn>      */ "meta2-6;3~",         "/window scroll_down");
        BIND(/* m-<home>      */ "meta-meta2-1~",      "/window scroll_top");
        BIND(/* m-<home>      */ "meta-meta2-7~",      "/window scroll_top");
        BIND(/* m-<home>      */ "meta2-1;3H"   ,      "/window scroll_top");
        BIND(/* m-<end>       */ "meta-meta2-4~",      "/window scroll_bottom");
        BIND(/* m-<end>       */ "meta-meta2-8~",      "/window scroll_bottom");
        BIND(/* m-<end>       */ "meta2-1;3F",         "/window scroll_bottom");
        BIND(/* m-n           */ "meta-n",             "/window scroll_next_highlight");
        BIND(/* m-p           */ "meta-p",             "/window scroll_previous_highlight");
        BIND(/* m-N           */ "meta-N",             "/bar toggle nicklist");
        BIND(/* <f9>          */ "meta2-20~",          "/bar scroll title * -30%");
        BIND(/* <f10>         */ "meta2-21~",          "/bar scroll title * +30%");
        BIND(/* <f11>         */ "meta2-23~",          "/bar scroll nicklist * -100%");
        BIND(/* <f12>         */ "meta2-24~",          "/bar scroll nicklist * +100%");
        BIND(/* c-<f11>       */ "meta2-23^",          "/bar scroll nicklist * -100%");
        BIND(/* c-<f11>       */ "meta2-23;5~",        "/bar scroll nicklist * -100%");
        BIND(/* c-<f12>       */ "meta2-24^",          "/bar scroll nicklist * +100%");
        BIND(/* c-<f12>       */ "meta2-24;5~",        "/bar scroll nicklist * +100%");
        BIND(/* m-<f11>       */ "meta2-23;3~",        "/bar scroll nicklist * b");
        BIND(/* m-<f11>       */ "meta-meta2-23~",     "/bar scroll nicklist * b");
        BIND(/* m-<f12>       */ "meta2-24;3~",        "/bar scroll nicklist * e");
        BIND(/* m-<f12>       */ "meta-meta2-24~",     "/bar scroll nicklist * e");
        BIND(/* ^L            */ "ctrl-L",             "/window refresh");
        BIND(/* <f7>          */ "meta2-18~",          "/window -1");
        BIND(/* <f8>          */ "meta2-19~",          "/window +1");
        BIND(/* m-w,m-<up>    */ "meta-wmeta-meta2-A", "/window up");
        BIND(/* m-w,m-<up>    */ "meta-wmeta2-1;3A",   "/window up");
        BIND(/* m-w,m-<down>  */ "meta-wmeta-meta2-B", "/window down");
        BIND(/* m-w,m-<down>  */ "meta-wmeta2-1;3B",   "/window down");
        BIND(/* m-w,m-<right> */ "meta-wmeta-meta2-C", "/window right");
        BIND(/* m-w,m-<right> */ "meta-wmeta2-1;3C",   "/window right");
        BIND(/* m-w,m-<left>  */ "meta-wmeta-meta2-D", "/window left");
        BIND(/* m-w,m-<left>  */ "meta-wmeta2-1;3D",   "/window left");
        BIND(/* m-w,m-b       */ "meta-wmeta-b",       "/window balance");
        BIND(/* m-w,m-s       */ "meta-wmeta-s",       "/window swap");
        BIND(/* m-z           */ "meta-z",             "/window zoom");
        BIND(/* m-=           */ "meta-=",             "/filter toggle");
        BIND(/* m--           */ "meta--",             "/filter toggle @");
        BIND(/* m-0           */ "meta-0",             "/buffer *10");
        BIND(/* m-1           */ "meta-1",             "/buffer *1");
        BIND(/* m-2           */ "meta-2",             "/buffer *2");
        BIND(/* m-3           */ "meta-3",             "/buffer *3");
        BIND(/* m-4           */ "meta-4",             "/buffer *4");
        BIND(/* m-5           */ "meta-5",             "/buffer *5");
        BIND(/* m-6           */ "meta-6",             "/buffer *6");
        BIND(/* m-7           */ "meta-7",             "/buffer *7");
        BIND(/* m-8           */ "meta-8",             "/buffer *8");
        BIND(/* m-9           */ "meta-9",             "/buffer *9");
        BIND(/* m-<           */ "meta-<",             "/buffer jump prev_visited");
        BIND(/* m->           */ "meta->",             "/buffer jump next_visited");
        BIND(/* m-/           */ "meta-/",             "/buffer jump last_displayed");
        BIND(/* m-l           */ "meta-l",             "/window bare");
        BIND(/* m-m           */ "meta-m",             "/mute mouse toggle");
        BIND(/* start paste   */ "meta2-200~",         "/input paste_start");
        BIND(/* end paste     */ "meta2-201~",         "/input paste_stop");

        /* bind meta-j + {01..99} to switch to buffers # > 10 */
        for (i = 1; i < 100; i++)
        {
            snprintf (key_str, sizeof (key_str), "meta-j%02d", i);
            snprintf (command, sizeof (command), "/buffer *%d", i);
            BIND(key_str, command);
        }
    }
    else if (context == GUI_KEY_CONTEXT_SEARCH)
    {
        BIND(/* <enter> */ "ctrl-M",  "/input search_stop_here");
        BIND(/* <enter> */ "ctrl-J",  "/input search_stop_here");
        BIND(/* ^Q      */ "ctrl-Q",  "/input search_stop");
        BIND(/* m-c     */ "meta-c",  "/input search_switch_case");
        BIND(/* ^R      */ "ctrl-R",  "/input search_switch_regex");
        BIND(/* <tab>   */ "ctrl-I",  "/input search_switch_where");
        BIND(/* <up>    */ "meta2-A", "/input search_previous");
        BIND(/* <down>  */ "meta2-B", "/input search_next");
    }
    else if (context == GUI_KEY_CONTEXT_CURSOR)
    {
        /* general & move */
        BIND(/* <enter>   */ "ctrl-M",                   "/cursor stop");
        BIND(/* <enter>   */ "ctrl-J",                   "/cursor stop");
        BIND(/* <up>      */ "meta2-A",                  "/cursor move up");
        BIND(/* <down>    */ "meta2-B",                  "/cursor move down");
        BIND(/* <left>    */ "meta2-D",                  "/cursor move left");
        BIND(/* <right>   */ "meta2-C",                  "/cursor move right");
        BIND(/* m-<up>    */ "meta-meta2-A",             "/cursor move area_up");
        BIND(/* m-<up>    */ "meta2-1;3A",               "/cursor move area_up");
        BIND(/* m-<down>  */ "meta-meta2-B",             "/cursor move area_down");
        BIND(/* m-<down>  */ "meta2-1;3B",               "/cursor move area_down");
        BIND(/* m-<left>  */ "meta-meta2-D",             "/cursor move area_left");
        BIND(/* m-<left>  */ "meta2-1;3D",               "/cursor move area_left");
        BIND(/* m-<right> */ "meta-meta2-C",             "/cursor move area_right");
        BIND(/* m-<right> */ "meta2-1;3C",               "/cursor move area_right");
        /* chat */
        BIND(/* m         */ "@chat:m",                  "hsignal:chat_quote_message;/cursor stop");
        BIND(/* q         */ "@chat:q",                  "hsignal:chat_quote_prefix_message;/cursor stop");
        BIND(/* Q         */ "@chat:Q",                  "hsignal:chat_quote_time_prefix_message;/cursor stop");
        /* nicklist */
        BIND(/* b         */ "@item(buffer_nicklist):b", "/window ${_window_number};/ban ${nick}");
        BIND(/* k         */ "@item(buffer_nicklist):k", "/window ${_window_number};/kick ${nick}");
        BIND(/* K         */ "@item(buffer_nicklist):K", "/window ${_window_number};/kickban ${nick}");
        BIND(/* q         */ "@item(buffer_nicklist):q", "/window ${_window_number};/query ${nick};/cursor stop");
        BIND(/* w         */ "@item(buffer_nicklist):w", "/window ${_window_number};/whois ${nick}");
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
        }

        if (key_str[0])
        {
            /*
             * send the signal "key_pressed" only if NOT reading a mouse event
             * or if the mouse code is valid UTF-8 (do not send partial mouse
             * code which is not UTF-8 valid)
             */
            if (!gui_mouse_event_pending || utf8_is_valid (key_str, -1, NULL))
            {
                (void) hook_signal_send ("key_pressed",
                                         WEECHAT_HOOK_SIGNAL_STRING, key_str);
            }

            if (gui_current_window->buffer->text_search != GUI_TEXT_SEARCH_DISABLED)
                input_old = (gui_current_window->buffer->input_buffer) ?
                    strdup (gui_current_window->buffer->input_buffer) : strdup ("");
            else
                input_old = NULL;
            old_buffer = gui_current_window->buffer;

            if ((gui_key_pressed (key_str) != 0) && (insert_ok)
                && (!gui_cursor_mode))
            {
                if (!paste || !undo_done)
                    gui_buffer_undo_snap (gui_current_window->buffer);
                gui_input_insert_string (gui_current_window->buffer, key_str);
                gui_input_text_changed_modifier_and_signal (gui_current_window->buffer,
                                                            (!paste || !undo_done) ? 1 : 0,
                                                            1); /* stop completion */
                undo_done = 1;
            }

            /* incremental text search in buffer */
            if ((old_buffer == gui_current_window->buffer)
                && (gui_current_window->buffer->text_search != GUI_TEXT_SEARCH_DISABLED)
                && ((input_old == NULL)
                    || (gui_current_window->buffer->input_buffer == NULL)
                    || (strcmp (input_old, gui_current_window->buffer->input_buffer) != 0)))
            {
                /*
                 * if following conditions are all true, then do not search
                 * again (search will not find any result and can take some time
                 * on a buffer with many lines):
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
                    /*
                     * do not search text in buffer, just alert about text not
                     * found
                     */
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
        if (gui_key_grab || gui_mouse_event_pending || !gui_key_combo_buffer[0])
            last_key_used = i;
    }

    if (last_key_used == gui_key_buffer_size - 1)
        gui_key_buffer_reset ();
    else if (last_key_used >= 0)
        gui_key_buffer_remove (0, last_key_used + 1);

    if (!gui_key_grab && !gui_mouse_event_pending)
        gui_key_combo_buffer[0] = '\0';
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
            /* ctrl-Y: accept paste */
            accept_paste = 1;
        }
        else if (gui_key_paste_pending && (buffer[i] == 14))
        {
            /* ctrl-N: cancel paste */
            cancel_paste = 1;
        }
        else
        {
            gui_key_buffer_add (buffer[i]);
            text_added_to_buffer = 1;
        }
    }

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

            /* replace tabs by spaces */
            gui_key_paste_replace_tabs ();

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
