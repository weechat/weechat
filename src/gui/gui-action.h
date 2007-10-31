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


#ifndef __WEECHAT_GUI_ACTION_H
#define __WEECHAT_GUI_ACTION_H 1

/* action functions */

extern void gui_action_clipboard_copy (char *, int);
extern void gui_action_clipboard_paste (char *);
extern void gui_action_return (char *);
extern void gui_action_tab (char *);
extern void gui_action_tab_previous (char *);
extern void gui_action_backspace (char *);
extern void gui_action_delete (char *);
extern void gui_action_delete_previous_word (char *);
extern void gui_action_delete_next_word (char *);
extern void gui_action_delete_begin_of_line (char *);
extern void gui_action_delete_end_of_line (char *);
extern void gui_action_delete_line (char *);
extern void gui_action_transpose_chars (char *);
extern void gui_action_home (char *);
extern void gui_action_end (char *);
extern void gui_action_left (char *);
extern void gui_action_previous_word (char *);
extern void gui_action_right (char *);
extern void gui_action_next_word (char *);
extern void gui_action_up (char *);
extern void gui_action_up_global (char *);
extern void gui_action_down (char *);
extern void gui_action_down_global (char *);
extern void gui_action_page_up (char *);
extern void gui_action_page_down (char *);
extern void gui_action_scroll_up (char *);
extern void gui_action_scroll_down (char *);
extern void gui_action_scroll_top (char *);
extern void gui_action_scroll_bottom (char *);
extern void gui_action_scroll_topic_left (char *);
extern void gui_action_scroll_topic_right (char *);
extern void gui_action_nick_beginning (char *);
extern void gui_action_nick_end (char *);
extern void gui_action_nick_page_up (char *);
extern void gui_action_nick_page_down (char *);
extern void gui_action_jump_smart (char *);
extern void gui_action_jump_dcc (char *);
extern void gui_action_jump_raw_data (char *);
extern void gui_action_jump_last_buffer (char *);
extern void gui_action_jump_previous_buffer (char *);
extern void gui_action_jump_server (char *);
extern void gui_action_jump_next_server (char *);
extern void gui_action_switch_server (char *);
extern void gui_action_scroll_previous_highlight (char *);
extern void gui_action_scroll_next_highlight (char *);
extern void gui_action_scroll_unread (char *);
extern void gui_action_set_unread (char *);
extern void gui_action_hotlist_clear (char *);
extern void gui_action_infobar_clear (char *);
extern void gui_action_refresh_screen (char *);
extern void gui_action_grab_key (char *);
extern void gui_action_insert_string (char *);
extern void gui_action_search_text (char *);

#endif /* gui-action.h */
