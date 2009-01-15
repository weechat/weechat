/*
 * Copyright (c) 2009 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_JABBER_UPGRADE_H
#define __WEECHAT_JABBER_UPGRADE_H 1

#define JABBER_UPGRADE_FILENAME "jabber"

/* For developers: please add new values ONLY AT THE END of enums */

enum t_jabber_upgrade_type
{
    JABBER_UPGRADE_TYPE_SERVER = 0,
    JABBER_UPGRADE_TYPE_MUC,
    JABBER_UPGRADE_TYPE_BUDDY,
};

extern int jabber_upgrade_save ();
extern int jabber_upgrade_load ();

#endif /* jabber-upgrade.h */
