/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_LOG_H
#define WEECHAT_PLUGIN_OTR_LOG_H

struct t_otr_peer_data;

extern int otr_log_buffer_is_logged (struct t_gui_buffer *buf);
extern int otr_log_buffer_log_level (struct t_gui_buffer *buf);
extern void otr_log_disable (ConnContext *c, struct t_otr_peer_data *pd);
extern void otr_log_restore (ConnContext *c, struct t_otr_peer_data *pd);
extern int otr_log_logger_level_update_cb (const void *pointer, void *data,
                                           const char *option,
                                           const char *value);

#endif /* WEECHAT_PLUGIN_OTR_LOG_H */
