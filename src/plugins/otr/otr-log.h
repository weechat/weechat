/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_LOG_H
#define WEECHAT_PLUGIN_OTR_LOG_H

struct peer_data;

extern int buf_is_logged (struct t_gui_buffer *buf);
extern int buf_log_level (struct t_gui_buffer *buf);
extern void log_disable (ConnContext *c, struct peer_data *pd);
extern void log_restore (ConnContext *c, struct peer_data *pd);
extern int logger_level_update_cb (const void *pointer, void *data,
                                   const char *option, const char *value);

#endif /* WEECHAT_PLUGIN_OTR_LOG_H */
