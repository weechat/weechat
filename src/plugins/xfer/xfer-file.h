/*
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_XFER_FILE_H
#define WEECHAT_PLUGIN_XFER_FILE_H

extern const char *xfer_file_search_crc32 (const char *filename);
extern void xfer_file_find_filename (struct t_xfer *xfer);
extern void xfer_file_calculate_speed (struct t_xfer *xfer, int ended);

#endif /* WEECHAT_PLUGIN_XFER_FILE_H */
