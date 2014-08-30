/*
 * Copyright (C) 2003-2014 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_DEBUG_H
#define WEECHAT_DEBUG_H 1

struct t_gui_window_tree;

extern void debug_sigsegv ();
extern void debug_windows_tree ();
extern void debug_memory ();
extern void debug_hdata ();
extern void debug_hooks ();
extern void debug_infolists ();
extern void debug_directories ();
extern void debug_time_start (const char *name);
extern void debug_time_end (int display);
extern void debug_init ();
extern void debug_end ();

#endif /* WEECHAT_DEBUG_H */
