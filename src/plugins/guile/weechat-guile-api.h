/*
 * SPDX-FileCopyrightText: 2011-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_GUILE_API_H
#define WEECHAT_PLUGIN_GUILE_API_H

extern int weechat_guile_api_buffer_input_data_cb (const void *pointer,
                                                   void *data,
                                                   struct t_gui_buffer *buffer,
                                                   const char *input_data);
extern int weechat_guile_api_buffer_close_cb (const void *pointer,
                                              void *data,
                                              struct t_gui_buffer *buffer);
extern void weechat_guile_api_module_init (void *data);

#endif /* WEECHAT_PLUGIN_GUILE_API_H */
