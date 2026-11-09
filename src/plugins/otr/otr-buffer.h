/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_BUFFER_H
#define WEECHAT_PLUGIN_OTR_BUFFER_H

extern void otr_buffer_debug (const char *format, ...);
extern void otr_buffer_print_str (struct t_gui_buffer *buffer, int color,
                                  const char *message);
extern void otr_buffer_error_core (const char *format, ...);
extern int otr_buffer_is_private (struct t_gui_buffer *buffer);
extern void otr_buffer_peer_print (const char *server, const char *nick,
                                   int color, const char *message);
extern void otr_buffer_ctx_print (ConnContext *context, int color,
                                  const char *format, ...);
extern void otr_buffer_ctx_hint (ConnContext *context, const char *message);
extern void otr_buffer_peer_hint (const char *server, const char *nick,
                                  const char *message);
extern int otr_buffer_switch_cb (const void *pointer, void *data,
                                 const char *signal, const char *type_data,
                                 void *signal_data);
extern int otr_buffer_closing_cb (const void *pointer, void *data,
                                  const char *signal, const char *type_data,
                                  void *signal_data);

#endif /* WEECHAT_PLUGIN_OTR_BUFFER_H */
