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

extern void gui_action_clipboard_copy (char *buffer, int size);
extern void gui_action_clipboard_paste (char *args);
extern void gui_action_return (char *args);
extern void gui_action_tab (char *args);
extern void gui_action_tab_previous (char *args);
extern void gui_action_backspace (char *args);
extern void gui_action_delete (char *args);
extern void gui_action_delete_previous_word (char *args);
extern void gui_action_delete_next_word (char *args);
extern void gui_action_delete_begin_of_line (char *args);
extern void gui_action_delete_end_of_line (char *args);
extern void gui_action_delete_line (char *args);
extern void gui_action_transpose_chars (char *args);
extern void gui_action_home (char *args);
extern void gui_action_end (char *args);
extern void gui_action_left (char *args);
extern void gui_action_previous_word (char *args);
extern void gui_action_right (char *args);
extern void gui_action_next_word (char *args);
extern void gui_action_up (char *args);
extern void gui_action_up_global (char *args);
extern void gui_action_down (char *args);
extern void gui_action_down_global (char *args);
extern void gui_action_page_up (char *args);
extern void gui_action_page_down (char *args);
extern void gui_action_scroll_up (char *args);
extern void gui_action_scroll_down (char *args);
extern void gui_action_scroll_top (char *args);
extern void gui_action_scroll_bottom (char *args);
extern void gui_action_scroll_topic_left (char *args);
extern void gui_action_scroll_topic_right (char *args);
extern void gui_action_nick_beginning (char *args);
extern void gui_action_nick_end (char *args);
extern void gui_action_nick_page_up (char *args);
extern void gui_action_nick_page_down (char *args);
extern void gui_action_jump_smart (char *args);
extern void gui_action_jump_dcc (char *args);
extern void gui_action_jump_raw_data (char *args);
extern void gui_action_jump_last_buffer (char *args);
extern void gui_action_jump_previous_buffer (char *args);
extern void gui_action_jump_server (char *args);
extern void gui_action_jump_next_server (char *args);
extern void gui_action_switch_server (char *args);
extern void gui_action_scroll_previous_highlight (char *args);
extern void gui_action_scroll_next_highlight (char *args);
extern void gui_action_scroll_unread (char *args);
extern void gui_action_set_unread (char *args);
extern void gui_action_hotlist_clear (char *args);
extern void gui_action_infobar_clear (char *args);
extern void gui_action_refresh_screen (char *args);
extern void gui_action_grab_key (char *args);
extern void gui_action_insert_string (char *args);
extern void gui_action_search_text (char *args);

#endif /* gui-action.h */
