/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* gui-gtk-keyboard.c: keyboard functions for Gtk GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../common/utf8.h"
#include "gui-gtk.h"

#ifdef PLUGINS
#include "../../plugins/plugins.h"
#endif


/*
 * gui_keyboard_default_bindings: create default key bindings
 */

void
gui_keyboard_default_bindings ()
{
    int i;
    char key_str[32], command[32];
    
    /* keys binded with internal functions */
    gui_keyboard_bind ( /* RC          */ "ctrl-M",             "return");
    gui_keyboard_bind ( /* RC          */ "ctrl-J",             "return");
    gui_keyboard_bind ( /* tab         */ "ctrl-I",             "tab");
    gui_keyboard_bind ( /* basckp      */ "ctrl-H",             "backspace");
    gui_keyboard_bind ( /* basckp      */ "ctrl-?",             "backspace");
    gui_keyboard_bind ( /* del         */ "meta2-3~",           "delete");
    gui_keyboard_bind ( /* ^K          */ "ctrl-K",             "delete_end_line");
    gui_keyboard_bind ( /* ^U          */ "ctrl-U",             "delete_beginning_line");
    gui_keyboard_bind ( /* ^W          */ "ctrl-W",             "delete_previous_word");
    gui_keyboard_bind ( /* ^Y          */ "ctrl-Y",             "clipboard_paste");
    gui_keyboard_bind ( /* ^T          */ "ctrl-T",             "transpose_chars");
    gui_keyboard_bind ( /* home        */ "meta2-1~",           "home");
    gui_keyboard_bind ( /* home        */ "meta2-H",            "home");
    gui_keyboard_bind ( /* home        */ "meta2-7~",           "home");
    gui_keyboard_bind ( /* ^A          */ "ctrl-A",             "home");
    gui_keyboard_bind ( /* end         */ "meta2-4~",           "end");
    gui_keyboard_bind ( /* end         */ "meta2-F",            "end");
    gui_keyboard_bind ( /* end         */ "meta2-8~",           "end");
    gui_keyboard_bind ( /* ^E          */ "ctrl-E",             "end");
    gui_keyboard_bind ( /* left        */ "meta2-D",            "left");
    gui_keyboard_bind ( /* right       */ "meta2-C",            "right");
    gui_keyboard_bind ( /* up          */ "meta2-A",            "up");
    gui_keyboard_bind ( /* ^up         */ "meta-Oa",            "up_global");
    gui_keyboard_bind ( /* down        */ "meta2-B",            "down");
    gui_keyboard_bind ( /* ^down       */ "meta-Ob",            "down_global");
    gui_keyboard_bind ( /* pgup        */ "meta2-5~",           "page_up");
    gui_keyboard_bind ( /* pgdn        */ "meta2-6~",           "page_down");
    gui_keyboard_bind ( /* m-pgup      */ "meta-meta2-5~",      "scroll_up");
    gui_keyboard_bind ( /* m-pgdn      */ "meta-meta2-6~",      "scroll_down");
    gui_keyboard_bind ( /* F10         */ "meta2-21~",          "infobar_clear");
    gui_keyboard_bind ( /* F11         */ "meta2-23~",          "nick_page_up");
    gui_keyboard_bind ( /* F12         */ "meta2-24~",          "nick_page_down");
    gui_keyboard_bind ( /* m-F11       */ "meta-meta2-1~",      "nick_beginning");
    gui_keyboard_bind ( /* m-F12       */ "meta-meta2-4~",      "nick_end");
    gui_keyboard_bind ( /* ^L          */ "ctrl-L",             "refresh");
    gui_keyboard_bind ( /* m-a         */ "meta-a",             "jump_smart");
    gui_keyboard_bind ( /* m-b         */ "meta-b",             "previous_word");
    gui_keyboard_bind ( /* ^left       */ "meta-Od",            "previous_word");
    gui_keyboard_bind ( /* m-d         */ "meta-d",             "delete_next_word");
    gui_keyboard_bind ( /* m-f         */ "meta-f",             "next_word");
    gui_keyboard_bind ( /* ^right      */ "meta-Oc",            "next_word");
    gui_keyboard_bind ( /* m-h         */ "meta-h",             "hotlist_clear");
    gui_keyboard_bind ( /* m-j,m-d     */ "meta-jmeta-d",       "jump_dcc");
    gui_keyboard_bind ( /* m-j,m-l     */ "meta-jmeta-l",       "jump_last_buffer");
    gui_keyboard_bind ( /* m-j,m-s     */ "meta-jmeta-s",       "jump_server");
    gui_keyboard_bind ( /* m-j,m-x     */ "meta-jmeta-x",       "jump_next_server");
    gui_keyboard_bind ( /* m-k         */ "meta-k",             "grab_key");
    gui_keyboard_bind ( /* m-n         */ "meta-n",             "scroll_next_highlight");
    gui_keyboard_bind ( /* m-p         */ "meta-p",             "scroll_previous_highlight");
    gui_keyboard_bind ( /* m-r         */ "meta-r",             "delete_line");
    gui_keyboard_bind ( /* m-s         */ "meta-s",             "switch_server");
    gui_keyboard_bind ( /* m-u         */ "meta-u",             "scroll_unread");
    
    /* keys binded with commands */
    gui_keyboard_bind ( /* m-left      */ "meta-meta2-D",       "/buffer -1");
    gui_keyboard_bind ( /* F5          */ "meta2-15~",          "/buffer -1");
    gui_keyboard_bind ( /* m-right     */ "meta-meta2-C",       "/buffer +1");
    gui_keyboard_bind ( /* F6          */ "meta2-17~",          "/buffer +1");
    gui_keyboard_bind ( /* F7          */ "meta2-18~",          "/window -1");
    gui_keyboard_bind ( /* F8          */ "meta2-19~",          "/window +1");
    gui_keyboard_bind ( /* m-w,m-up    */ "meta-wmeta-meta2-A", "/window up");
    gui_keyboard_bind ( /* m-w,m-down  */ "meta-wmeta-meta2-B", "/window down");
    gui_keyboard_bind ( /* m-w,m-left  */ "meta-wmeta-meta2-D", "/window left");
    gui_keyboard_bind ( /* m-w,m-right */ "meta-wmeta-meta2-C", "/window right");
    gui_keyboard_bind ( /* m-0         */ "meta-0",             "/buffer 10");
    gui_keyboard_bind ( /* m-1         */ "meta-1",             "/buffer 1");
    gui_keyboard_bind ( /* m-2         */ "meta-2",             "/buffer 2");
    gui_keyboard_bind ( /* m-3         */ "meta-3",             "/buffer 3");
    gui_keyboard_bind ( /* m-4         */ "meta-4",             "/buffer 4");
    gui_keyboard_bind ( /* m-5         */ "meta-5",             "/buffer 5");
    gui_keyboard_bind ( /* m-6         */ "meta-6",             "/buffer 6");
    gui_keyboard_bind ( /* m-7         */ "meta-7",             "/buffer 7");
    gui_keyboard_bind ( /* m-8         */ "meta-8",             "/buffer 8");
    gui_keyboard_bind ( /* m-9         */ "meta-9",             "/buffer 9");
    
    /* bind meta-j + {01..99} to switch to buffers # > 10 */
    for (i = 1; i < 100; i++)
    {
        sprintf (key_str, "meta-j%02d", i);
        sprintf (command, "/buffer %d", i);
        gui_keyboard_bind (key_str, command);
    }
}

/*
 * gui_keyboard_grab_end: insert grabbed key in input buffer
 */

void
gui_keyboard_grab_end ()
{
    char *expanded_key;

    /* get expanded name (for example: ^U => ctrl-u) */
    expanded_key = gui_keyboard_get_expanded_name (gui_key_buffer);
    
    if (expanded_key)
    {
        if (gui_current_window->buffer->has_input)
        {
            gui_insert_string_input (gui_current_window, expanded_key, -1);
            gui_current_window->buffer->input_buffer_pos += utf8_strlen (expanded_key);
            gui_input_draw (gui_current_window->buffer, 1);
        }
        free (expanded_key);
    }
    
    /* end grab mode */
    gui_key_grab = 0;
    gui_key_grab_count = 0;
    gui_key_buffer[0] = '\0';
}

/*
 * gui_keyboard_read: read keyboard chars
 */

void
gui_keyboard_read ()
{
    /* TODO: write this function for Gtk */
}
