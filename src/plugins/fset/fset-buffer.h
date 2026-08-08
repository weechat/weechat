/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_FSET_BUFFER_H
#define WEECHAT_PLUGIN_FSET_BUFFER_H

#define FSET_BUFFER_NAME "fset"

struct t_fset_option;

extern struct t_gui_buffer *fset_buffer;
extern int fset_buffer_selected_line;

extern void fset_buffer_set_title (void);
extern int fset_buffer_display_option (struct t_fset_option *fset_option);
extern void fset_buffer_refresh (int clear);
extern void fset_buffer_set_current_line (int line);
extern void fset_buffer_check_line_outside_window (void);
extern int fset_buffer_window_scrolled_cb (const void *pointer,
                                           void *data,
                                           const char *signal,
                                           const char *type_data,
                                           void *signal_data);
extern void fset_buffer_set_keys (struct t_hashtable *hashtable);
extern void fset_buffer_set_localvar_filter (void);
extern void fset_buffer_open (void);
extern int fset_buffer_init (void);
extern void fset_buffer_end (void);

#endif /* WEECHAT_PLUGIN_FSET_BUFFER_H */
