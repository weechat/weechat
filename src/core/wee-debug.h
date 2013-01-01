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

#ifndef __WEECHAT_DEBUG_H
#define __WEECHAT_DEBUG_H 1

struct t_gui_window_tree;

extern void debug_sigsegv ();
extern void debug_windows_tree ();
extern void debug_memory ();
extern void debug_hdata ();
extern void debug_hooks ();
extern void debug_infolists ();
extern void debug_init ();

#endif /* __WEECHAT_DEBUG_H */
