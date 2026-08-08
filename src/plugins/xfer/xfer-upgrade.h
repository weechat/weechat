/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
