/*
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_DEBUG_H
#define WEECHAT_DEBUG_H

#include <sys/time.h>

struct t_gui_window_tree;

extern long long debug_long_callbacks;

extern void debug_build_info ();
extern void debug_sigsegv_cb (int);
extern void debug_windows_tree ();
extern void debug_memory ();
extern void debug_hdata ();
extern void debug_hooks ();
extern void debug_hooks_plugin_types (const char *plugin_name,
                                      const char **hook_types);
extern void debug_infolists ();
extern void debug_directories ();
extern void debug_display_time_elapsed (struct timeval *time1,
                                        struct timeval *time2,
                                        const char *message,
                                        int display);
extern void debug_unicode (const char *string);
extern void debug_init ();
extern void debug_end ();

#endif /* WEECHAT_DEBUG_H */
