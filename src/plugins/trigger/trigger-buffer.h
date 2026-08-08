/*
 * SPDX-FileCopyrightText: 2014-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_TRIGGER_BUFFER_H
#define WEECHAT_PLUGIN_TRIGGER_BUFFER_H

#define TRIGGER_BUFFER_NAME "monitor"

struct t_trigger_context;

extern struct t_gui_buffer *trigger_buffer;

extern void trigger_buffer_set_callbacks (void);
extern void trigger_buffer_open (const char *filter, int switch_to_buffer);
extern int trigger_buffer_display_trigger (struct t_trigger *trigger,
                                           struct t_trigger_context *context);
extern void trigger_buffer_end (void);

#endif /* WEECHAT_PLUGIN_TRIGGER_BUFFER_H */
