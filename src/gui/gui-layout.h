/*
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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

#ifndef __WEECHAT_GUI_LAYOUT_H
#define __WEECHAT_GUI_LAYOUT_H 1

/* layout structures */

struct t_gui_layout_buffer
{
    char *plugin_name;
    char *buffer_name;
    int number;
    struct t_gui_layout_buffer *prev_layout; /* link to previous layout     */
    struct t_gui_layout_buffer *next_layout; /* link to next layout         */
};

struct t_gui_layout_window
{
    int internal_id;                    /* used to save/read layout from    */
                                        /* config (to find parent)          */
    struct t_gui_layout_window *parent_node; /* pointer to parent node      */

    /* node info */
    int split_pct;                      /* % of split size (child1)         */
    int split_horiz;                    /* 1 if horizontal, 0 if vertical   */
    struct t_gui_layout_window *child1; /* first child, NULL if a leaf      */
    struct t_gui_layout_window *child2; /* second child, NULL if leaf       */

    /* leaf info */
    char *plugin_name;
    char *buffer_name;
};

/* layout variables */

extern struct t_gui_layout_buffer *gui_layout_buffers;
extern struct t_gui_layout_buffer *last_gui_layout_buffer;
extern struct t_gui_layout_window *gui_layout_windows;

/* layout functions */

extern void gui_layout_buffer_remove_all (struct t_gui_layout_buffer **layout_buffers,
                                          struct t_gui_layout_buffer **last_layout_buffer);
extern void gui_layout_buffer_reset (struct t_gui_layout_buffer **layout_buffers,
                                          struct t_gui_layout_buffer **last_layout_buffer);
extern struct t_gui_layout_buffer *gui_layout_buffer_add (struct t_gui_layout_buffer **layout_buffers,
                                                          struct t_gui_layout_buffer **last_layout_buffer,
                                                          const char *plugin_name,
                                                          const char *buffer_name,
                                                          int number);
extern void gui_layout_buffer_save (struct t_gui_layout_buffer **layout_buffers,
                                    struct t_gui_layout_buffer **last_layout_buffer);
extern void gui_layout_buffer_get_number (struct t_gui_layout_buffer *layout_buffers,
                                          const char *plugin_name,
                                          const char *buffer_name,
                                          int *layout_number,
                                          int *layout_number_merge_order);
extern void gui_layout_buffer_get_number_all (struct t_gui_layout_buffer *layout_buffers);
extern void gui_layout_buffer_apply (struct t_gui_layout_buffer *layout_buffers);

extern void gui_layout_window_remove_all (struct t_gui_layout_window **layout_windows);
extern void gui_layout_window_reset (struct t_gui_layout_window **layout_windows);
extern struct t_gui_layout_window *gui_layout_window_search_by_id (struct t_gui_layout_window *layout_windows,
                                                                   int id);
extern struct t_gui_layout_window *gui_layout_window_add (struct t_gui_layout_window **layout_windows,
                                                          int internal_id,
                                                          struct t_gui_layout_window *parent,
                                                          int split_pct,
                                                          int split_horiz,
                                                          const char *plugin_name,
                                                          const char *buffer_name);
extern int gui_layout_window_save (struct t_gui_layout_window **layout_windows);
extern void gui_layout_window_apply (struct t_gui_layout_window *layout_windows,
                                     int internal_id_current_window);
extern void gui_layout_window_check_buffer (struct t_gui_buffer *buffer);
extern void gui_layout_save_on_exit ();
extern int gui_layout_buffer_add_to_infolist (struct t_infolist *infolist,
                                              struct t_gui_layout_buffer *layout_buffer);
extern int gui_layout_window_add_to_infolist (struct t_infolist *infolist,
                                              struct t_gui_layout_window *layout_window);
extern void gui_layout_print_log ();

#endif /* __WEECHAT_GUI_LAYOUT_H */
