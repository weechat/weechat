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

#ifndef WEECHAT_COMPLETION_H
#define WEECHAT_COMPLETION_H

struct t_gui_buffer;
struct t_gui_completion;

extern int completion_list_add_filename_cb (const void *pointer,
                                            void *data,
                                            const char *completion_item,
                                            struct t_gui_buffer *buffer,
                                            struct t_gui_completion *completion);
extern void completion_init (void);

#endif /* WEECHAT_COMPLETION_H */
