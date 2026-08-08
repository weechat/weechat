/*
 * SPDX-FileCopyrightText: 2014-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_EXEC_BUFFER_H
#define WEECHAT_PLUGIN_EXEC_BUFFER_H

extern void exec_buffer_set_callbacks (void);
extern struct t_gui_buffer *exec_buffer_new (const char *name,
                                             int free_content,
                                             int clear_buffer,
                                             int switch_to_buffer);

#endif /* WEECHAT_PLUGIN_EXEC_BUFFER_H */
