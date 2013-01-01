/*
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WEECHAT_UPGRADE_H
#define __WEECHAT_UPGRADE_H 1

#include "wee-upgrade-file.h"

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

int upgrade_weechat_save ();
int upgrade_weechat_load ();
void upgrade_weechat_end ();

#endif /* __WEECHAT_UPGRADE_H */
