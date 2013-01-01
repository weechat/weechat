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

#ifndef __WEECHAT_RELAY_UPGRADE_H
#define __WEECHAT_RELAY_UPGRADE_H 1

#define RELAY_UPGRADE_FILENAME "relay"

/* For developers: please add new values ONLY AT THE END of enums */

enum t_relay_upgrade_type
{
    RELAY_UPGRADE_TYPE_CLIENT = 0,
    RELAY_UPGRADE_TYPE_RAW_MESSAGE,
    RELAY_UPGRADE_TYPE_SERVER,
};

extern int relay_upgrade_save ();
extern int relay_upgrade_load ();

#endif /* __WEECHAT_RELAY_UPGRADE_H */
