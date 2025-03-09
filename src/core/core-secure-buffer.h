/*
 * Copyright (C) 2013-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_SECURE_BUFFER_H
#define WEECHAT_SECURE_BUFFER_H

#define SECURE_BUFFER_NAME "secured_data"

extern struct t_gui_buffer *secure_buffer;
extern int secure_buffer_display_values;

extern void secure_buffer_display (void);
extern void secure_buffer_assign (void);
extern void secure_buffer_open (void);

#endif /* WEECHAT_SECURE_BUFFER_H */
