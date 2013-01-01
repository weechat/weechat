/*
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WEECHAT_GUI_MAIN_H
#define __WEECHAT_GUI_MAIN_H 1

/* main functions (GUI dependent) */

extern void gui_main_loop ();
extern void gui_main_pre_init (int *argc, char **argv[]);
extern void gui_main_init ();
extern void gui_main_end (int clean_exit);

/* terminal functions (GUI dependent) */
extern void gui_term_set_eat_newline_glitch (int value);

#endif /* __WEECHAT_GUI_MAIN_H */
