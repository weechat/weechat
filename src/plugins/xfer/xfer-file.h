/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_XFER_FILE_H
#define WEECHAT_PLUGIN_XFER_FILE_H

extern const char *xfer_file_search_crc32 (const char *filename);
extern void xfer_file_find_filename (struct t_xfer *xfer);
extern void xfer_file_calculate_speed (struct t_xfer *xfer, int ended);

#endif /* WEECHAT_PLUGIN_XFER_FILE_H */
