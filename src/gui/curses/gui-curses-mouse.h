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

#ifndef WEECHAT_GUI_CURSES_MOUSE_H
#define WEECHAT_GUI_CURSES_MOUSE_H

#define MOUSE_CODE_UTF8_MOTION(code) ((code >= 64) && (code < 96))
#define MOUSE_CODE_UTF8_END(code)    ((code == '#') || (code == '3')    \
                                      || (code == '+') || (code == ';'))

#endif /* WEECHAT_GUI_CURSES_MOUSE_H */
