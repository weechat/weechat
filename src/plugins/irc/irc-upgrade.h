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
    IRC_UPGRADE_TYPE_REDIRECT_PATTERN,
    IRC_UPGRADE_TYPE_REDIRECT,
    IRC_UPGRADE_TYPE_NOTIFY,
};

extern int irc_upgrade_save ();
extern int irc_upgrade_load ();

#endif /* __WEECHAT_IRC_UPGRADE_H */
