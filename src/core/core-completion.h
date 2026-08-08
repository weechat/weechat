/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_COMPLETION_H
#define WEECHAT_COMPLETION_H

struct t_gui_buffer;
struct t_gui_completion;

extern int completion_list_add_filename_cb (const void *pointer,
                                            void *data,
                                            const char *completion_item,
                                            struct t_gui_buffer *buffer,
                                            struct t_gui_completion *completion);
extern void completion_init (void);

#endif /* WEECHAT_COMPLETION_H */
