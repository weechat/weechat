/*
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_GUI_LAYOUT_H
#define WEECHAT_GUI_LAYOUT_H

#define GUI_LAYOUT_DEFAULT_NAME "default"

/* layouts reserved for internal use */
#define GUI_LAYOUT_ZOOM    "_zoom"
#define GUI_LAYOUT_UPGRADE "_upgrade"

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

struct t_gui_layout
{
    char *name;                        /* name of layout                    */
    struct t_gui_layout_buffer *layout_buffers;     /* layout for buffers   */
    struct t_gui_layout_buffer *last_layout_buffer; /* last buffer layout   */
    struct t_gui_layout_window *layout_windows;     /* layout for windows   */
    int internal_id;                   /* for unique id in each window      */
    int internal_id_current_window;    /* id of current window              */
    struct t_gui_layout *prev_layout;  /* pointer to previous layout        */
    struct t_gui_layout *next_layout;  /* pointer to next layout            */
};

/* layout variables */

extern struct t_gui_layout *gui_layouts;
extern struct t_gui_layout *last_gui_layout;
extern struct t_gui_layout *gui_layout_current;

/* layout functions */

extern struct t_gui_layout *gui_layout_search (const char *name);
extern struct t_gui_layout *gui_layout_alloc (const char *name);
extern int gui_layout_add (struct t_gui_layout *layout);
extern void gui_layout_rename (struct t_gui_layout *layout, const char *new_name);
extern void gui_layout_buffer_remove_all (struct t_gui_layout *layout);
extern void gui_layout_buffer_reset (void);
extern struct t_gui_layout_buffer *gui_layout_buffer_add (struct t_gui_layout *layout,
                                                          const char *plugin_name,
                                                          const char *buffer_name,
                                                          int number);
extern void gui_layout_buffer_get_number (struct t_gui_layout *layout,
                                          const char *plugin_name,
                                          const char *buffer_name,
                                          int *layout_number,
                                          int *layout_number_merge_order);
extern void gui_layout_buffer_get_number_all (struct t_gui_layout *layout);
extern void gui_layout_buffer_store (struct t_gui_layout *layout);
extern void gui_layout_buffer_apply (struct t_gui_layout *layout);

extern void gui_layout_window_remove_all (struct t_gui_layout *layout);
extern void gui_layout_window_reset (void);
extern struct t_gui_layout_window *gui_layout_window_search_by_id (struct t_gui_layout_window *layout_window,
                                                                   int id);
extern struct t_gui_layout_window *gui_layout_window_add (struct t_gui_layout_window **layout_window,
                                                          int internal_id,
                                                          struct t_gui_layout_window *parent,
                                                          int split_pct,
                                                          int split_horiz,
                                                          const char *plugin_name,
                                                          const char *buffer_name);
extern void gui_layout_window_store (struct t_gui_layout *layout);
extern int gui_layout_window_check_buffer (struct t_gui_window *window);
extern void gui_layout_window_assign_buffer (struct t_gui_buffer *buffer);
extern void gui_layout_window_apply (struct t_gui_layout *layout,
                                     int internal_id_current_window);
extern void gui_layout_store_on_exit (void);
extern void gui_layout_free (struct t_gui_layout *layout);
extern void gui_layout_remove (struct t_gui_layout *layout);
extern void gui_layout_remove_all (void);
extern struct t_hdata *gui_layout_hdata_layout_buffer_cb (const void *pointer,
                                                          void *data,
                                                          const char *hdata_name);
extern struct t_hdata *gui_layout_hdata_layout_window_cb (const void *pointer,
                                                          void *data,
                                                          const char *hdata_name);
extern struct t_hdata *gui_layout_hdata_layout_cb (const void *pointer,
                                                   void *data,
                                                   const char *hdata_name);
extern int gui_layout_buffer_add_to_infolist (struct t_infolist *infolist,
                                              struct t_gui_layout_buffer *layout_buffer);
extern int gui_layout_window_add_to_infolist (struct t_infolist *infolist,
                                              struct t_gui_layout_window *layout_window);
extern int gui_layout_add_to_infolist (struct t_infolist *infolist,
                                       struct t_gui_layout *layout);
extern void gui_layout_print_log (void);

#endif /* WEECHAT_GUI_LAYOUT_H */
