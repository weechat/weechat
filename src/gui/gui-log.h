/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __WEECHAT_GUI_LOG_H
#define __WEECHAT_GUI_LOG_H 1

/* log functions */

extern void gui_log_write_date (struct t_gui_buffer *);
extern void gui_log_write_line (struct t_gui_buffer *, char *);
extern void gui_log_write (struct t_gui_buffer *, char *);
extern void gui_log_start (struct t_gui_buffer *);
extern void gui_log_end (struct t_gui_buffer *);

#endif /* gui-log.h */
