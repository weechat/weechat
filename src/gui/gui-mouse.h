/*
 * SPDX-FileCopyrightText: 2011-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_GUI_MOUSE_H
#define WEECHAT_GUI_MOUSE_H

/* Mouse variables */

extern int gui_mouse_enabled;
extern int gui_mouse_debug;
extern int gui_mouse_grab;
extern int gui_mouse_event_pending;
extern int gui_mouse_event_index;
extern int gui_mouse_event_x[2];
extern int gui_mouse_event_y[2];
extern char gui_mouse_event_button;

/* Mouse functions */

extern void gui_mouse_debug_set (int debug);
extern void gui_mouse_event_reset (void);

/* Mouse functions (GUI dependent) */

extern void gui_mouse_enable (void);
extern void gui_mouse_disable (void);
extern void gui_mouse_display_state (void);
extern void gui_mouse_grab_init (int area);
extern int gui_mouse_event_size (const char *key);
extern void gui_mouse_event_process (const char *key);

#endif /* WEECHAT_GUI_MOUSE_H */
