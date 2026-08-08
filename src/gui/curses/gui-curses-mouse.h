/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_GUI_CURSES_MOUSE_H
#define WEECHAT_GUI_CURSES_MOUSE_H

#define MOUSE_CODE_UTF8_MOTION(code) ((code >= 64) && (code < 96))
#define MOUSE_CODE_UTF8_END(code)    ((code == '#') || (code == '3')    \
                                      || (code == '+') || (code == ';'))

#endif /* WEECHAT_GUI_CURSES_MOUSE_H */
