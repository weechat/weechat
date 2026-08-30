/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_GUI_MAIN_H
#define WEECHAT_GUI_MAIN_H

/* Main functions (GUI dependent) */

extern void gui_main_get_password (const char **prompt,
                                   char *password, int size);
extern void gui_main_debug_libs (void);
extern void gui_main_end (int clean_exit);

/* Terminal functions (GUI dependent) */
extern void gui_term_set_eat_newline_glitch (int value);
extern int gui_term_theme_is_light (void);

#endif /* WEECHAT_GUI_MAIN_H */
