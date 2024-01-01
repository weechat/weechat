/*
 * Copyright (C) 2023-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_IRC_LIST_H
#define WEECHAT_PLUGIN_IRC_LIST_H

#include <regex.h>

#define IRC_LIST_MOUSE_HSIGNAL "irc_list_mouse"

struct t_irc_server;

struct t_irc_list_channel
{
    char *name;                        /* channel name                      */
    char *name2;                       /* channel name without prefix ('#') */
    int users;                         /* number of users in the channel    */
    char *topic;                       /* channel topic                     */
};

struct t_irc_list
{
    struct t_gui_buffer *buffer;       /* buffer for /list                  */
    struct t_arraylist *channels;      /* channels received in /list reply  */
    struct t_arraylist *filter_channels; /* filtered channels               */
    int name_max_length;               /* max length for channel name       */
    char *filter;                      /* filter for channels               */
    char *sort;                        /* sort for channels                 */
    char **sort_fields;                /* sort fields                       */
    int sort_fields_count;             /* number of sort fields             */
    int selected_line;                 /* selected line                     */
};

extern void irc_list_buffer_set_title (struct t_irc_server *server);
extern void irc_list_buffer_refresh (struct t_irc_server *server, int clear);
extern int irc_list_window_scrolled_cb (const void *pointer, void *data,
                                        const char *signal,
                                        const char *type_data,
                                        void *signal_data);
extern void irc_list_move_line_relative (struct t_irc_server *server,
                                         int num_lines);
extern void irc_list_move_line_absolute (struct t_irc_server *server,
                                         int line_number);
extern void irc_list_scroll_horizontal (struct t_irc_server *server,
                                        int percent);
extern void irc_list_join_channel (struct t_irc_server *server);
extern int irc_list_buffer_input_data (struct t_gui_buffer *buffer,
                                       const char *input_data);
extern struct t_gui_buffer *irc_list_create_buffer (struct t_irc_server *server);
extern int irc_list_hsignal_redirect_list_cb (const void *pointer,
                                              void *data,
                                              const char *signal,
                                              struct t_hashtable *hashtable);
extern void irc_list_reset (struct t_irc_server *server);
extern struct t_irc_list *irc_list_alloc ();
extern void irc_list_free_data (struct t_irc_server *server);
extern void irc_list_free (struct t_irc_server *server);
extern struct t_hdata *irc_list_hdata_list_channel_cb (const void *pointer,
                                                       void *data,
                                                       const char *hdata_name);
extern struct t_hdata *irc_list_hdata_list_cb (const void *pointer, void *data,
                                               const char *hdata_name);
extern void irc_list_init ();
extern void irc_list_end ();

#endif /* WEECHAT_PLUGIN_IRC_LIST_H */
