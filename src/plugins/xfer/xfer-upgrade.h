/*
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_XFER_UPGRADE_H
#define WEECHAT_PLUGIN_XFER_UPGRADE_H

#define XFER_UPGRADE_FILENAME "xfer"

/* For developers: please add new values ONLY AT THE END of enums */

enum t_xfer_upgrade_type
{
    XFER_UPGRADE_TYPE_XFER = 0,
};

extern int xfer_upgrade_save (void);
extern int xfer_upgrade_load (void);

#endif /* WEECHAT_PLUGIN_XFER_UPGRADE_H */
