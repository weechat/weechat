/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_GUI_NICK_H
#define WEECHAT_GUI_NICK_H

extern char *gui_nick_find_color_name (const char *nickname, int case_range,
                                       const char *colors);
extern char *gui_nick_find_color (const char *nickname, int case_range,
                                  const char *colors);

#endif /* WEECHAT_GUI_NICK_H */
