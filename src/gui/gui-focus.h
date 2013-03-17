/*
 * Copyright (C) 2011-2013 Sebastien Helleu <flashcode@flashtux.org>
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

#ifndef __WEECHAT_GUI_FOCUS_H
#define __WEECHAT_GUI_FOCUS 1

/* focus structures */

struct t_gui_focus_info
{
    int x, y;                          /* (x,y) on screen                   */
    struct t_gui_window *window;       /* window found                      */
    int chat;                          /* 1 for chat area, otherwise 0      */
    struct t_gui_line *chat_line;      /* line in chat area                 */
    int chat_line_x;                   /* x in line                         */
    char *chat_word;                   /* word at (x,y)                     */
    char *chat_bol;                    /* beginning of line until (x,y)     */
    char *chat_eol;                    /* (x,y) until end of line           */
    struct t_gui_bar_window *bar_window; /* bar window found                */
    char *bar_item;                    /* bar item found                    */
    int bar_item_line;                 /* line in bar item                  */
    int bar_item_col;                  /* column in bar item                */
};

/* focus functions */

extern struct t_gui_focus_info *gui_focus_get_info (int x, int y);
extern void gui_focus_free_info (struct t_gui_focus_info *focus_info);
extern struct t_hashtable *gui_focus_to_hashtable (struct t_gui_focus_info *focus_info,
                                                   const char *key);

#endif /* __WEECHAT_GUI_FOCUS_H */
