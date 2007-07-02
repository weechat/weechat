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


#ifndef __WEECHAT_GUI_H
#define __WEECHAT_GUI_H 1

#include "gui-color.h"
#include "gui-panel.h"
#include "gui-buffer.h"
#include "gui-window.h"
#include "gui-keyboard.h"


#define gui_printf(buffer, fmt, argz...) \
    gui_printf_internal(buffer, 1, MSG_TYPE_INFO, NULL, fmt, ##argz)

#define gui_printf_type(buffer, type, fmt, argz...) \
    gui_printf_internal(buffer, 1, type, NULL, fmt, ##argz)

#define gui_printf_type_nick(buffer, type, nick, fmt, argz...) \
    gui_printf_internal(buffer, 1, type, nick, fmt, ##argz)

#define gui_printf_nolog(buffer, fmt, argz...) \
    gui_printf_internal(buffer, 1, MSG_TYPE_INFO | MSG_TYPE_NOLOG, NULL, fmt, ##argz)

#define gui_printf_nolog_notime(buffer, fmt, argz...) \
    gui_printf_internal(buffer, 0, MSG_TYPE_NOLOG, NULL, fmt, ##argz)


typedef struct t_gui_infobar t_gui_infobar;

struct t_gui_infobar
{
    int color;                      /* text color                           */
    char *text;                     /* infobar text                         */
    int remaining_time;             /* delay (sec) before erasing this text */
                                    /* if < 0, text is never erased (except */
                                    /* by user action to erase it)          */
    t_gui_infobar *next_infobar;    /* next message for infobar             */
};

/* GUI variables */

extern int gui_init_ok;
extern int gui_ok;
extern int gui_add_hotlist;
extern t_gui_infobar *gui_infobar;
extern char *gui_input_clipboard;
extern time_t gui_last_activity_time;

/* GUI independent functions */

/* window */
extern int gui_window_tree_init (t_gui_window *);
extern void gui_window_tree_node_to_leaf (t_gui_window_tree *, t_gui_window *);
extern void gui_window_tree_free (t_gui_window_tree **);
extern t_gui_window *gui_window_new (t_gui_window *, int, int, int, int, int, int);
extern void gui_window_free (t_gui_window *);
extern void gui_window_switch_server (t_gui_window *);
extern void gui_window_switch_previous (t_gui_window *);
extern void gui_window_switch_next (t_gui_window *);
extern void gui_window_switch_by_buffer (t_gui_window *, int);
extern void gui_window_print_log (t_gui_window *);

/* buffer */
extern t_gui_buffer *gui_buffer_servers_search ();
extern t_gui_buffer *gui_buffer_new (t_gui_window *, void *, void *, int, int);
extern t_gui_buffer *gui_buffer_search (char *, char *);
extern t_gui_buffer *gui_buffer_search_by_number (int);
extern t_gui_window *gui_buffer_find_window (t_gui_buffer *);
extern void gui_buffer_find_context (void *, void *,
                                     t_gui_window **, t_gui_buffer **);
extern int gui_buffer_is_scrolled (t_gui_buffer *);
extern t_gui_buffer *gui_buffer_get_dcc (t_gui_window *);
extern void gui_buffer_clear (t_gui_buffer *);
extern void gui_buffer_clear_all ();
extern void gui_buffer_free (t_gui_buffer *, int);
extern t_gui_line *gui_buffer_line_new (t_gui_buffer *, time_t);
extern int gui_buffer_line_search (t_gui_line *, char *, int);
extern void gui_buffer_merge_servers (t_gui_window *);
extern void gui_buffer_split_server (t_gui_window *);
extern void gui_buffer_switch_previous (t_gui_window *);
extern void gui_buffer_switch_next (t_gui_window *);
extern void gui_buffer_switch_dcc (t_gui_window *);
extern void gui_buffer_switch_raw_data (t_gui_window *);
extern t_gui_buffer *gui_buffer_switch_by_number (t_gui_window *, int);
extern void gui_buffer_move_to_number (t_gui_buffer *, int);
extern void gui_buffer_search_start (t_gui_window *);
extern void gui_buffer_search_restart (t_gui_window *);
extern void gui_buffer_search_stop (t_gui_window *);
extern int gui_buffer_search_text (t_gui_window *);
extern void gui_buffer_print_log (t_gui_buffer *);

/* panel */
extern int gui_panel_global_get_size (t_gui_panel *, int);
extern t_gui_panel *gui_panel_new (char *, int, int, int, int);
extern void gui_panel_free (t_gui_panel *);
extern void gui_panel_print_log ();

/* action */
extern void gui_action_clipboard_copy (char *, int);
extern void gui_action_clipboard_paste (t_gui_window *, char *);
extern void gui_action_return (t_gui_window *, char *);
extern void gui_action_tab (t_gui_window *, char *);
extern void gui_action_tab_previous (t_gui_window *, char *);
extern void gui_action_backspace (t_gui_window *, char *);
extern void gui_action_delete (t_gui_window *, char *);
extern void gui_action_delete_previous_word (t_gui_window *, char *);
extern void gui_action_delete_next_word (t_gui_window *, char *);
extern void gui_action_delete_begin_of_line (t_gui_window *, char *);
extern void gui_action_delete_end_of_line (t_gui_window *, char *);
extern void gui_action_delete_line (t_gui_window *, char *);
extern void gui_action_transpose_chars (t_gui_window *, char *);
extern void gui_action_home (t_gui_window *, char *);
extern void gui_action_end (t_gui_window *, char *);
extern void gui_action_left (t_gui_window *, char *);
extern void gui_action_previous_word (t_gui_window *, char *);
extern void gui_action_right (t_gui_window *, char *);
extern void gui_action_next_word (t_gui_window *, char *);
extern void gui_action_up (t_gui_window *, char *);
extern void gui_action_up_global (t_gui_window *, char *);
extern void gui_action_down (t_gui_window *, char *);
extern void gui_action_down_global (t_gui_window *, char *);
extern void gui_action_page_up (t_gui_window *, char *);
extern void gui_action_page_down (t_gui_window *, char *);
extern void gui_action_scroll_up (t_gui_window *, char *);
extern void gui_action_scroll_down (t_gui_window *, char *);
extern void gui_action_scroll_top (t_gui_window *, char *);
extern void gui_action_scroll_bottom (t_gui_window *, char *);
extern void gui_action_scroll_topic_left (t_gui_window *, char *);
extern void gui_action_scroll_topic_right (t_gui_window *, char *);
extern void gui_action_nick_beginning (t_gui_window *, char *);
extern void gui_action_nick_end (t_gui_window *, char *);
extern void gui_action_nick_page_up (t_gui_window *, char *);
extern void gui_action_nick_page_down (t_gui_window *, char *);
extern void gui_action_jump_smart (t_gui_window *, char *);
extern void gui_action_jump_dcc (t_gui_window *, char *);
extern void gui_action_jump_raw_data (t_gui_window *, char *);
extern void gui_action_jump_last_buffer (t_gui_window *, char *);
extern void gui_action_jump_server (t_gui_window *, char *);
extern void gui_action_jump_next_server (t_gui_window *, char *);
extern void gui_action_switch_server (t_gui_window *, char *);
extern void gui_action_scroll_previous_highlight (t_gui_window *, char *);
extern void gui_action_scroll_next_highlight (t_gui_window *, char *);
extern void gui_action_scroll_unread (t_gui_window *, char *);
extern void gui_action_hotlist_clear (t_gui_window *, char *);
extern void gui_action_infobar_clear (t_gui_window *, char *);
extern void gui_action_refresh_screen (t_gui_window *, char *);
extern void gui_action_grab_key (t_gui_window *, char *);
extern void gui_action_insert_string (t_gui_window *, char *);
extern void gui_action_search_text (t_gui_window *, char *);

/* key */
extern void gui_keyboard_init ();
extern void gui_keyboard_init_grab ();
extern char *gui_keyboard_get_internal_code (char *);
extern char *gui_keyboard_get_expanded_name (char *);
extern t_gui_key *gui_keyboard_search (char *);
extern t_gui_key_func *gui_keyboard_function_search_by_name (char *);
extern char *gui_keyboard_function_search_by_ptr (t_gui_key_func *);
extern t_gui_key *gui_keyboard_bind (char *, char *);
extern int gui_keyboard_unbind (char *);
extern int gui_keyboard_pressed (char *);
extern void gui_keyboard_free (t_gui_key *);
extern void gui_keyboard_free_all ();

/* log */
extern void gui_log_write_date (t_gui_buffer *);
extern void gui_log_write_line (t_gui_buffer *, char *);
extern void gui_log_write (t_gui_buffer *, char *);
extern void gui_log_start (t_gui_buffer *);
extern void gui_log_end (t_gui_buffer *);

/* other */
extern int gui_word_strlen (t_gui_window *, char *);
extern int gui_word_real_pos (t_gui_window *, char *, int);
extern void gui_printf_internal (t_gui_buffer *, int, int, char *, char *, ...);
extern void gui_printf_raw_data (void *, int, int, char *);
extern void gui_infobar_printf (int, int, char *, ...);
extern void gui_infobar_printf_from_buffer (t_gui_buffer *, int, int, char *, char *, ...);
extern void gui_infobar_remove ();
extern void gui_infobar_remove_all ();
extern void gui_input_optimize_size (t_gui_buffer *);
extern void gui_input_init_color_mask (t_gui_buffer *);
extern void gui_input_move (t_gui_buffer *, char *, char *, int );
extern void gui_input_complete (t_gui_window *);
extern void gui_exec_action_dcc (t_gui_window *, char *);
extern void gui_exec_action_raw_data (t_gui_window *, char *);
extern int gui_insert_string_input (t_gui_window *, char *, int);

/* GUI dependent functions */

/* color */
extern int gui_color_assign (int *, char *);
extern char *gui_color_get_name (int);
extern unsigned char *gui_color_decode (unsigned char *, int);
extern unsigned char *gui_color_decode_for_user_entry (unsigned char *);
extern unsigned char *gui_color_encode (unsigned char *);
extern void gui_color_init_pairs ();
extern void gui_color_rebuild_weechat();

/* keyboard */
extern void gui_keyboard_default_bindings ();

/* chat */
extern void gui_chat_draw_title (t_gui_buffer *, int);
extern char *gui_chat_word_get_next_char (t_gui_window *, unsigned char *, int, int *);
extern void gui_chat_draw (t_gui_buffer *, int);
extern void gui_chat_draw_line (t_gui_buffer *, t_gui_line *);

/* status bar */
extern void gui_infobar_draw_time (t_gui_buffer *);
extern void gui_infobar_draw (t_gui_buffer *, int);

/* info bar */
extern void gui_status_draw (t_gui_buffer *, int);

/* input */
extern void gui_input_draw (t_gui_buffer *, int);

/* nicklist */
extern void gui_nicklist_draw (t_gui_buffer *, int, int);

/* window */
extern int gui_window_get_width ();
extern int gui_window_get_height ();
extern int gui_window_objects_init (t_gui_window *);
extern void gui_window_objects_free (t_gui_window *, int);
extern int gui_window_calculate_pos_size (t_gui_window *, int);
extern void gui_window_redraw_buffer (t_gui_buffer *);
extern void gui_window_switch_to_buffer (t_gui_window *, t_gui_buffer *);
extern void gui_window_page_up (t_gui_window *);
extern void gui_window_page_down (t_gui_window *);
extern void gui_window_scroll_up (t_gui_window *);
extern void gui_window_scroll_down (t_gui_window *);
extern void gui_window_scroll_top (t_gui_window *);
extern void gui_window_scroll_bottom (t_gui_window *);
extern void gui_window_scroll_topic_left (t_gui_window *);
extern void gui_window_scroll_topic_right (t_gui_window *);
extern void gui_window_nick_beginning (t_gui_window *);
extern void gui_window_nick_end (t_gui_window *);
extern void gui_window_nick_page_up (t_gui_window *);
extern void gui_window_nick_page_down (t_gui_window *);
extern void gui_window_init_subwindows (t_gui_window *);
extern void gui_window_refresh_windows ();
extern void gui_window_split_horiz (t_gui_window *, int);
extern void gui_window_split_vertic (t_gui_window *, int);
extern void gui_window_resize (t_gui_window *, int);
extern int gui_window_merge (t_gui_window *);
extern void gui_window_merge_all (t_gui_window *);
extern void gui_window_switch_up (t_gui_window *);
extern void gui_window_switch_down (t_gui_window *);
extern void gui_window_switch_left (t_gui_window *);
extern void gui_window_switch_right (t_gui_window *);
extern void gui_window_refresh_screen ();
extern void gui_window_set_title ();
extern void gui_window_reset_title ();
extern void gui_window_objects_print_log (t_gui_window *);

/* panel */
extern int gui_panel_window_new (t_gui_panel *, t_gui_window *);
extern void gui_panel_window_free (void *);

/* main */
extern void gui_main_loop ();
extern void gui_main_pre_init (int *, char **[]);
extern void gui_main_init ();
extern void gui_main_end ();

#endif /* gui.h */
