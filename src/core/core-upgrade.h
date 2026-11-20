/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_UPGRADE_H
#define WEECHAT_UPGRADE_H

#include "core-upgrade-file.h"

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

extern int upgrade_pending;
extern char *upgrade_pending_binary;
extern int upgrade_pending_quit;

int upgrade_weechat_save (void);
int upgrade_weechat_load (void);
void upgrade_weechat_end (void);
int upgrade_weechat_check_background (void);
void upgrade_weechat_schedule (char *binary, int quit);
void upgrade_weechat_cancel (void);
void upgrade_weechat_exec (void);

#endif /* WEECHAT_UPGRADE_H */
