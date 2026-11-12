/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

#ifndef WEECHAT_UPGRADE_H
#define WEECHAT_UPGRADE_H

#include "core-upgrade-file.h"

#define WEECHAT_UPGRADE_FILENAME "weechat"

/* For developers: please add new values ONLY AT THE END of enums */

enum t_upgrade_weechat_type
{
    UPGRADE_WEECHAT_TYPE_HISTORY = 0,
    UPGRADE_WEECHAT_TYPE_BUFFER,
    UPGRADE_WEECHAT_TYPE_NICKLIST,
    UPGRADE_WEECHAT_TYPE_BUFFER_LINE,
    UPGRADE_WEECHAT_TYPE_MISC,
    UPGRADE_WEECHAT_TYPE_HOTLIST,
    UPGRADE_WEECHAT_TYPE_LAYOUT_WINDOW,
};

extern int upgrade_pending;
extern char *upgrade_pending_binary;
extern int upgrade_pending_quit;

int upgrade_weechat_save (void);
int upgrade_weechat_load (void);
void upgrade_weechat_end (void);
int upgrade_weechat_check_background (void);
void upgrade_weechat_schedule (char *binary, int quit);
void upgrade_weechat_cancel (void);
void upgrade_weechat_exec (void);

#endif /* WEECHAT_UPGRADE_H */
