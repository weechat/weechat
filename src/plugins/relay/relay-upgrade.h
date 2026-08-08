/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_RELAY_UPGRADE_H
#define WEECHAT_PLUGIN_RELAY_UPGRADE_H

#define RELAY_UPGRADE_FILENAME "relay"

/* For developers: please add new values ONLY AT THE END of enums */

enum t_relay_upgrade_type
{
    RELAY_UPGRADE_TYPE_CLIENT = 0,
    RELAY_UPGRADE_TYPE_RAW_MESSAGE,
    RELAY_UPGRADE_TYPE_SERVER,
};

extern int relay_upgrade_save (int force_disconnected_state);
extern int relay_upgrade_load (void);

#endif /* WEECHAT_PLUGIN_RELAY_UPGRADE_H */
