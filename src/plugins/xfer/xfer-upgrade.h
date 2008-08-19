/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_XFER_UPGRADE_H
#define __WEECHAT_XFER_UPGRADE_H 1

#define XFER_UPGRADE_FILENAME "xfer"

/* For developers: please add new values ONLY AT THE END of enums */

enum t_xfer_upgrade_type
{
    XFER_UPGRADE_TYPE_XFER = 0,
};

extern int xfer_upgrade_save ();
extern int xfer_upgrade_load ();

#endif /* xfer-upgrade.h */
