/*
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WEECHAT_GUI_MAIN_H
#define WEECHAT_GUI_MAIN_H

/* main functions (GUI dependent) */

extern void gui_main_get_password (const char **prompt,
                                   char *password, int size);
extern void gui_main_debug_libs (void);
extern void gui_main_end (int clean_exit);

/* terminal functions (GUI dependent) */
extern void gui_term_set_eat_newline_glitch (int value);

#endif /* WEECHAT_GUI_MAIN_H */
