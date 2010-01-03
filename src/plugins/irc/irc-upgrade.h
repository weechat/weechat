/*
 * Copyright (c) 2003-2010 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __WEECHAT_IRC_UPGRADE_H
#define __WEECHAT_IRC_UPGRADE_H 1

#define IRC_UPGRADE_FILENAME "irc"

/* For developers: please add new values ONLY AT THE END of enums */

enum t_irc_upgrade_type
{
    IRC_UPGRADE_TYPE_SERVER = 0,
    IRC_UPGRADE_TYPE_CHANNEL,
    IRC_UPGRADE_TYPE_NICK,
    IRC_UPGRADE_TYPE_RAW_MESSAGE,
};

extern int irc_upgrade_save ();
extern int irc_upgrade_load ();

#endif /* irc-upgrade.h */
