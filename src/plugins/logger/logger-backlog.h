/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_LOGGER_BACKLOG_H
#define WEECHAT_PLUGIN_LOGGER_BACKLOG_H

extern int logger_backlog_signal_cb (const void *pointer, void *data,
                                     const char *signal,
                                     const char *type_data, void *signal_data);

#endif /* WEECHAT_PLUGIN_LOGGER_BACKLOG_H */
