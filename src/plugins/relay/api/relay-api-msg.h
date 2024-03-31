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

#ifndef WEECHAT_PLUGIN_RELAY_API_MSG_H
#define WEECHAT_PLUGIN_RELAY_API_MSG_H

enum t_relay_api_colors;

extern int relay_api_msg_send_json (struct t_relay_client *client,
                                    int return_code,
                                    const char *message,
                                    const char *body_type,
                                    cJSON *json_body);
extern int relay_api_msg_send_error_json (struct t_relay_client *client,
                                          int return_code,
                                          const char *message,
                                          const char *headers,
                                          const char *format, ...);
extern int relay_api_msg_send_event (struct t_relay_client *client,
                                     const char *name,
                                     struct t_gui_buffer *buffer,
                                     const char *body_type,
                                     cJSON *json_body);
extern cJSON *relay_api_msg_buffer_to_json (struct t_gui_buffer *buffer,
                                            long lines,
                                            int nicks,
                                            enum t_relay_api_colors colors);
extern cJSON *relay_api_msg_lines_to_json (struct t_gui_buffer *pointer,
                                           long lines,
                                           enum t_relay_api_colors colors);
extern cJSON *relay_api_msg_line_data_to_json (struct t_gui_line_data *line_data,
                                               enum t_relay_api_colors colors);
extern cJSON *relay_api_msg_nick_to_json (struct t_gui_nick *nick);
extern cJSON *relay_api_msg_nick_group_to_json (struct t_gui_nick_group *nick_group);
extern cJSON *relay_api_msg_hotlist_to_json (struct t_gui_hotlist *hotlist);

#endif /* WEECHAT_PLUGIN_RELAY_API_MSG_H */
