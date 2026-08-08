/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_IRC_UPGRADE_H
#define WEECHAT_PLUGIN_IRC_UPGRADE_H

#define IRC_UPGRADE_FILENAME "irc"

/* For developers: please add new values ONLY AT THE END of enums */

enum t_irc_upgrade_type
{
    IRC_UPGRADE_TYPE_SERVER = 0,
    IRC_UPGRADE_TYPE_CHANNEL,
    IRC_UPGRADE_TYPE_NICK,
    IRC_UPGRADE_TYPE_RAW_MESSAGE,
    IRC_UPGRADE_TYPE_REDIRECT_PATTERN,
    IRC_UPGRADE_TYPE_REDIRECT,
    IRC_UPGRADE_TYPE_NOTIFY,
    IRC_UPGRADE_TYPE_MODELIST,
    IRC_UPGRADE_TYPE_MODELIST_ITEM,
};

extern int irc_upgrading;

extern int irc_upgrade_save (int force_disconnected_state);
extern int irc_upgrade_load (void);

#endif /* WEECHAT_PLUGIN_IRC_UPGRADE_H */
