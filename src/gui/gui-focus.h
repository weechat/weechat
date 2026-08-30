/*
 * SPDX-FileCopyrightText: 2011-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_GUI_FOCUS_H
#define WEECHAT_GUI_FOCUS_H

/* Focus structures */

struct t_gui_focus_info
{
    int x, y;                          /* (x,y) on screen                   */
    struct t_gui_window *window;       /* window found                      */
    struct t_gui_buffer *buffer;       /* buffer found                      */
    int chat;                          /* 1 for chat area, otherwise 0      */
    struct t_gui_line *chat_line;      /* line in chat area                 */
    int chat_line_x;                   /* x in line                         */
    char *chat_word;                   /* word at (x,y)                     */
    char *chat_focused_line;           /* line at (x,y)                     */
    char *chat_focused_line_bol;       /* beg. of focused line until (x,y)  */
    char *chat_focused_line_eol;       /* (x,y) until end of focused line   */
    char *chat_bol;                    /* beginning of line until (x,y)     */
    char *chat_eol;                    /* (x,y) until end of line           */
    struct t_gui_bar_window *bar_window; /* bar window found                */
    char *bar_item;                    /* bar item found                    */
    int bar_item_line;                 /* line in bar item                  */
    int bar_item_col;                  /* column in bar item                */
};

/* Focus functions */

extern struct t_gui_focus_info *gui_focus_get_info (int x, int y);
extern void gui_focus_free_info (struct t_gui_focus_info *focus_info);
extern struct t_hashtable *gui_focus_to_hashtable (struct t_gui_focus_info *focus_info,
                                                   const char *key);
extern struct t_hashtable *gui_focus_info_hashtable_gui_focus_info_cb (const void *pointer,
                                                                       void *data,
                                                                       const char *info_name,
                                                                       struct t_hashtable *hashtable);

#endif /* WEECHAT_GUI_FOCUS_H */
