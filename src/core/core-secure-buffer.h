/*
 * SPDX-FileCopyrightText: 2013-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_SECURE_BUFFER_H
#define WEECHAT_SECURE_BUFFER_H

#define SECURE_BUFFER_NAME "secured_data"

extern struct t_gui_buffer *secure_buffer;
extern int secure_buffer_display_values;

extern void secure_buffer_display (void);
extern void secure_buffer_assign (void);
extern void secure_buffer_open (void);

#endif /* WEECHAT_SECURE_BUFFER_H */
