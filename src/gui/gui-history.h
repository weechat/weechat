/*
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_GUI_HISTORY_H
#define WEECHAT_GUI_HISTORY_H

struct t_gui_buffer;

struct t_gui_history
{
    char *text;                        /* text or command (entered by user) */
    struct t_gui_history *next_history;/* link to next text/command         */
    struct t_gui_history *prev_history;/* link to previous text/command     */
};

extern struct t_gui_history *gui_history;
extern struct t_gui_history *last_gui_history;
extern struct t_gui_history *gui_history_ptr;

extern void gui_history_buffer_add (struct t_gui_buffer *buffer,
                                    const char *string);
extern void gui_history_global_add (const char *string);
extern void gui_history_add (struct t_gui_buffer *buffer, const char *string);
extern int gui_history_search (struct t_gui_buffer *buffer,
                               struct t_gui_history *history);
extern void gui_history_global_free ();
extern void gui_history_buffer_free (struct t_gui_buffer *buffer);
extern struct t_hdata *gui_history_hdata_history_cb (const void *pointer,
                                                     void *data,
                                                     const char *hdata_name);
extern int gui_history_add_to_infolist (struct t_infolist *infolist,
                                        struct t_gui_history *history);

#endif /* WEECHAT_GUI_HISTORY_H */
