/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef __WEECHAT_LOG_H
#define __WEECHAT_LOG_H 1

#include "../irc/irc.h"
#include "../gui/gui.h"

extern void log_write_date (t_gui_buffer *);
extern void log_write_line (t_gui_buffer *, char *);
extern void log_write (t_gui_buffer *, char *);
extern void log_start (t_gui_buffer *);
extern void log_end (t_gui_buffer *);

#endif /* log.h */
