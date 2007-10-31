/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* gui-curses-keyboard.c: keyboard functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-utf8.h"
#include "../../core/wee-string.h"
#include "../gui-keyboard.h"
#include "../gui-input.h"
#include "../gui-completion.h"
#include "../gui-window.h"
#include "gui-curses.h"


/*
 * gui_keyboard_default_bindings: create default key bindings
 */

void
gui_keyboard_default_bindings ()
{
    int i;
    char key_str[32], command[32];
    
    /* keys bound with internal functions */
    gui_keyboard_bind ( /* RC          */ "ctrl-M",             "return");
    gui_keyboard_bind ( /* RC          */ "ctrl-J",             "return");
    gui_keyboard_bind ( /* tab         */ "ctrl-I",             "tab");
    gui_keyboard_bind ( /* s-tab       */ "meta2-Z",            "tab_previous");
    gui_keyboard_bind ( /* basckpace   */ "ctrl-H",             "backspace");
    gui_keyboard_bind ( /* basckpace   */ "ctrl-?",             "backspace");
    gui_keyboard_bind ( /* del         */ "meta2-3~",           "delete");
    gui_keyboard_bind ( /* ^D          */ "ctrl-D",             "delete");
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
    gui_keyboard_bind ( /* ^B          */ "ctrl-B",             "left");
    gui_keyboard_bind ( /* right       */ "meta2-C",            "right");
    gui_keyboard_bind ( /* ^F          */ "ctrl-F",             "right");
    gui_keyboard_bind ( /* up          */ "meta2-A",            "up");
    gui_keyboard_bind ( /* ^up         */ "meta-Oa",            "up_global");
    gui_keyboard_bind ( /* down        */ "meta2-B",            "down");
    gui_keyboard_bind ( /* ^down       */ "meta-Ob",            "down_global");
    gui_keyboard_bind ( /* pgup        */ "meta2-5~",           "page_up");
    gui_keyboard_bind ( /* pgup        */ "meta2-I",            "page_up");
    gui_keyboard_bind ( /* pgdn        */ "meta2-6~",           "page_down");
    gui_keyboard_bind ( /* pgdn        */ "meta2-G",            "page_down");
    gui_keyboard_bind ( /* m-pgup      */ "meta-meta2-5~",      "scroll_up");
    gui_keyboard_bind ( /* m-pgdn      */ "meta-meta2-6~",      "scroll_down");
    gui_keyboard_bind ( /* m-home      */ "meta-meta2-1~",      "scroll_top");
    gui_keyboard_bind ( /* m-end       */ "meta-meta2-4~",      "scroll_bottom");
    gui_keyboard_bind ( /* F9          */ "meta2-20~",          "scroll_topic_left");
    gui_keyboard_bind ( /* F10         */ "meta2-21~",          "scroll_topic_right");
    gui_keyboard_bind ( /* F11         */ "meta2-23~",          "nick_page_up");
    gui_keyboard_bind ( /* F12         */ "meta2-24~",          "nick_page_down");
    gui_keyboard_bind ( /* m-F11       */ "meta-meta2-23~",     "nick_beginning");
    gui_keyboard_bind ( /* m-F12       */ "meta-meta2-24~",     "nick_end");
    gui_keyboard_bind ( /* ^L          */ "ctrl-L",             "refresh");
    gui_keyboard_bind ( /* m-a         */ "meta-a",             "jump_smart");
    gui_keyboard_bind ( /* m-b         */ "meta-b",             "previous_word");
    gui_keyboard_bind ( /* ^left       */ "meta-Od",            "previous_word");
    gui_keyboard_bind ( /* m-d         */ "meta-d",             "delete_next_word");
    gui_keyboard_bind ( /* m-f         */ "meta-f",             "next_word");
    gui_keyboard_bind ( /* ^right      */ "meta-Oc",            "next_word");
    gui_keyboard_bind ( /* m-h         */ "meta-h",             "hotlist_clear");
    gui_keyboard_bind ( /* m-i         */ "meta-i"   ,          "infobar_clear");
    gui_keyboard_bind ( /* m-j,m-d     */ "meta-jmeta-d",       "jump_dcc");
    gui_keyboard_bind ( /* m-j,m-l     */ "meta-jmeta-l",       "jump_last_buffer");
    gui_keyboard_bind ( /* m-j,m-p     */ "meta-jmeta-p",       "jump_previous_buffer");
    gui_keyboard_bind ( /* m-j,m-r     */ "meta-jmeta-r",       "jump_raw_data");
    gui_keyboard_bind ( /* m-j,m-s     */ "meta-jmeta-s",       "jump_server");
    gui_keyboard_bind ( /* m-j,m-x     */ "meta-jmeta-x",       "jump_next_server");
    gui_keyboard_bind ( /* m-k         */ "meta-k",             "grab_key");
    gui_keyboard_bind ( /* m-n         */ "meta-n",             "scroll_next_highlight");
    gui_keyboard_bind ( /* m-p         */ "meta-p",             "scroll_previous_highlight");
    gui_keyboard_bind ( /* m-r         */ "meta-r",             "delete_line");
    gui_keyboard_bind ( /* m-s         */ "meta-s",             "switch_server");
    gui_keyboard_bind ( /* m-u         */ "meta-u",             "scroll_unread");
    gui_keyboard_bind ( /* ^S^U        */ "ctrl-Sctrl-U",       "set_unread");
    gui_keyboard_bind ( /* ^R          */ "ctrl-R",             "search_text");
    gui_keyboard_bind ( /* ^Cb         */ "ctrl-Cb",            "insert \\x02");
    gui_keyboard_bind ( /* ^Cc         */ "ctrl-Cc",            "insert \\x03");
    gui_keyboard_bind ( /* ^Co         */ "ctrl-Co",            "insert \\x0F");
    gui_keyboard_bind ( /* ^Cr         */ "ctrl-Cr",            "insert \\x12");
    gui_keyboard_bind ( /* ^Cu         */ "ctrl-Cu",            "insert \\x15");
    
    /* keys bound with commands */
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
 * gui_keyboard_read: read keyboard chars
 */

void
gui_keyboard_read ()
{
    int key, accept_paste, cancel_paste, text_added_to_buffer, paste_lines;
    
    accept_paste = 0;
    cancel_paste = 0;
    text_added_to_buffer = 0;
    
    while (1)
    {
        key = getch ();
        
        if (key == ERR)
            break;
        
#ifdef KEY_RESIZE
        if (key == KEY_RESIZE)
            continue;
#endif
        if (gui_keyboard_paste_pending)
        {
            /* ctrl-Y: accept paste */
            if (key == 25)
            {
                accept_paste = 1;
                break;
            }
            /* ctrl-N: cancel paste */
            else if (key == 14)
            {
                cancel_paste = 1;
                break;
            }
        }
        
        gui_keyboard_buffer_add (key);
        text_added_to_buffer = 1;
    }
    
    if (gui_keyboard_paste_pending)
    {
        /* user is ok for pasting text, let's paste! */
        if (accept_paste)
        {
            gui_keyboard_paste_accept ();
            gui_input_draw (gui_current_window->buffer, 1);
        }
        /* user doesn't want to paste text: clear whole buffer! */
        else if (cancel_paste)
        {
            gui_keyboard_paste_cancel ();
            gui_input_draw (gui_current_window->buffer, 1);
        }
        else if (text_added_to_buffer)
            gui_input_draw (gui_current_window->buffer, 1);
    }
    else
    {
        /* detect user paste or large amount of text
           if so, ask user what to do */
        if (cfg_look_paste_max_lines > 0)
        {
            paste_lines = gui_keyboard_get_paste_lines ();
            if (paste_lines > cfg_look_paste_max_lines)
            {
                gui_keyboard_paste_pending = 1;
                gui_input_draw (gui_current_window->buffer, 1);
            }
        }
    }
}

/*
 * gui_keyboard_flush: flush keyboard buffer
 */

void
gui_keyboard_flush ()
{
    int i, key, insert_ok, input_draw;
    char key_str[32], *key_utf, *input_old;
    
    /* if there's no paste pending, then we use buffer and do actions
       according to keys */
    if (!gui_keyboard_paste_pending)
    {
        if (gui_keyboard_buffer_size > 0)
            gui_keyboard_last_activity_time = time (NULL);
        
        if (gui_key_grab && (gui_key_grab_count > 0))
            gui_keyboard_grab_end ();
        
        for (i = 0; i < gui_keyboard_buffer_size; i++)
        {
            key = gui_keyboard_buffer[i];
            
            insert_ok = 1;
            
            if (key < 32)
            {
                insert_ok = 0;
                key_str[0] = '^';
                key_str[1] = (char) key + '@';
                key_str[2] = '\0';
            }
            else if (key == 127)
            {
                key_str[0] = '^';
                key_str[1] = '?';
                key_str[2] = '\0';
            }
            else
            {
                if (local_utf8)
                {
                    /* 1 char: 0vvvvvvv */
                    if (key < 0x80)
                    {
                        key_str[0] = (char) key;
                        key_str[1] = '\0';
                    }
                    /* 2 chars: 110vvvvv 10vvvvvv */
                    else if ((key & 0xE0) == 0xC0)
                    {
                        key_str[0] = (char) key;
                        if (i < gui_keyboard_buffer_size - 1)
                        {
                            key_str[1] = (char) (gui_keyboard_buffer[++i]);
                            key_str[2] = '\0';
                        }
                        else
                            key_str[1] = '\0';
                    }
                    /* 3 chars: 1110vvvv 10vvvvvv 10vvvvvv */
                    else if ((key & 0xF0) == 0xE0)
                    {
                        key_str[0] = (char) key;
                        if (i < gui_keyboard_buffer_size - 1)
                        {
                            key_str[1] = (char) (gui_keyboard_buffer[++i]);
                            if (i < gui_keyboard_buffer_size - 1)
                            {
                                key_str[2] = (char) (gui_keyboard_buffer[++i]);
                                key_str[3] = '\0';
                            }
                            else
                                key_str[2] = '\0';
                        }
                        else
                            key_str[1] = '\0';
                    }
                    /* 4 chars: 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv */
                    else if ((key & 0xF8) == 0xF0)
                    {
                        key_str[0] = (char) key;
                        if (i < gui_keyboard_buffer_size - 1)
                        {
                            key_str[1] = (char) (gui_keyboard_buffer[++i]);
                            if (i < gui_keyboard_buffer_size - 1)
                            {
                                key_str[2] = (char) (gui_keyboard_buffer[++i]);
                                if (i < gui_keyboard_buffer_size - 1)
                                {
                                    key_str[3] = (char) (gui_keyboard_buffer[++i]);
                                    key_str[4] = '\0';
                                }
                                else
                                    key_str[3] = '\0';
                            }
                            else
                                key_str[2] = '\0';
                        }
                        else
                            key_str[1] = '\0';
                    }
                }
                else
                {
                    key_str[0] = (char) key;
                    key_str[1] = '\0';
                    
                    /* convert input to UTF-8 is user is not using UTF-8 as locale */
                    if (!local_utf8)
                    {
                        key_utf = string_iconv_to_internal (NULL, key_str);
                        strncpy (key_str, key_utf, sizeof (key_str));
                        key_str[sizeof (key_str) - 1] = '\0';
                    }
                }
            }
            
            if (strcmp (key_str, "^") == 0)
            {
                key_str[1] = '^';
                key_str[2] = '\0';
            }
            
            /*gui_printf (gui_current_window->buffer,
                          "gui_input_read: key = %s (%d)\n", key_str, key);*/
            
            if (gui_current_window->buffer->text_search != GUI_TEXT_SEARCH_DISABLED)
                input_old = (gui_current_window->buffer->input_buffer) ?
                    strdup (gui_current_window->buffer->input_buffer) : strdup ("");
            else
                input_old = NULL;
            
            input_draw = 0;
            
            if ((gui_keyboard_pressed (key_str) != 0) && (insert_ok))
            {
                if (strcmp (key_str, "^^") == 0)
                    key_str[1] = '\0';
                
                /*switch (gui_current_window->buffer->type)
                {
                    case GUI_BUFFER_TYPE_STANDARD:*/
                        gui_input_insert_string (gui_current_window->buffer,
                                                 key_str, -1);
                        if (gui_current_window->buffer->completion)
                            gui_current_window->buffer->completion->position = -1;
                        input_draw = 1;
                        //break;
                    /*case GUI_BUFFER_TYPE_DCC:
                        gui_exec_action_dcc (gui_current_window, key_str);
                        break;
                    case GUI_BUFFER_TYPE_RAW_DATA:
                        gui_exec_action_raw_data (gui_current_window, key_str);
                        break;
                }*/
            }
            
            /* incremental text search in buffer */
            if ((gui_current_window->buffer->text_search != GUI_TEXT_SEARCH_DISABLED)
                && ((input_old == NULL) || (gui_current_window->buffer->input_buffer == NULL)
                    || (strcmp (input_old, gui_current_window->buffer->input_buffer) != 0)))
            {
                gui_window_search_restart (gui_current_window);
                input_draw = 1;
            }
            
            if (input_draw)
                gui_input_draw (gui_current_window->buffer, 0);
            
            if (input_old)
                free (input_old);
        }
        
        gui_keyboard_buffer_reset ();
    }
}
