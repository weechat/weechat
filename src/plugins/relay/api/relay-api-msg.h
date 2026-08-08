/*
 * SPDX-FileCopyrightText: 2023-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_RELAY_API_MSG_H
#define WEECHAT_PLUGIN_RELAY_API_MSG_H

enum t_relay_api_colors;

extern int relay_api_msg_send_json (struct t_relay_client *client,
                                    int return_code,
                                    const char *message,
                                    const char *headers,
                                    const char *body_type,
                                    cJSON *json_body);
extern int relay_api_msg_send_error_json (struct t_relay_client *client,
                                          int return_code,
                                          const char *message,
                                          const char *headers,
                                          const char *format, ...);
extern int relay_api_msg_send_event (struct t_relay_client *client,
                                     const char *name,
                                     long long buffer_id,
                                     const char *body_type,
                                     cJSON *json_body);
extern cJSON *relay_api_msg_buffer_to_json (struct t_gui_buffer *buffer,
                                            long lines,
                                            long lines_free,
                                            int nicks,
                                            enum t_relay_api_colors colors);
extern cJSON *relay_api_msg_key_to_json (struct t_gui_key *key);
extern cJSON *relay_api_msg_keys_to_json (struct t_gui_buffer *buffer);
extern cJSON *relay_api_msg_line_data_to_json (struct t_gui_line_data *line_data,
                                               enum t_relay_api_colors colors);
extern cJSON *relay_api_msg_lines_to_json (struct t_gui_buffer *buffer,
                                           long lines,
                                           enum t_relay_api_colors colors);
extern cJSON *relay_api_msg_nick_to_json (struct t_gui_nick *nick,
                                          enum t_relay_api_colors colors);
extern cJSON *relay_api_msg_nick_group_to_json (struct t_gui_nick_group *nick_group,
                                                enum t_relay_api_colors colors);
extern cJSON *relay_api_msg_completion_to_json (struct t_gui_completion *completion);
extern cJSON *relay_api_msg_hotlist_to_json (struct t_gui_hotlist *hotlist);
extern cJSON *relay_api_msg_script_to_json (struct t_hdata *hdata, void *script,
                                            const char *extension);

#endif /* WEECHAT_PLUGIN_RELAY_API_MSG_H */
