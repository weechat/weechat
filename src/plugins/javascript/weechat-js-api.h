/*
 * SPDX-FileCopyrightText: 2013 Koka El Kiwi <kokakiwi@kokakiwi.net>
 * SPDX-FileCopyrightText: 2015-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_JS_API_H
#define WEECHAT_PLUGIN_JS_API_H

extern int weechat_js_api_buffer_input_data_cb (const void *pointer,
                                                void *data,
                                                struct t_gui_buffer *buffer,
                                                const char *input_data);
extern int weechat_js_api_buffer_close_cb (const void *pointer,
                                           void *data,
                                           struct t_gui_buffer *buffer);

#endif /* WEECHAT_PLUGIN_JS_API_H */
