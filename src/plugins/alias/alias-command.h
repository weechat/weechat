/*
 * SPDX-FileCopyrightText: 2013-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_ALIAS_COMMAND_H
#define WEECHAT_PLUGIN_ALIAS_COMMAND_H

#define ALIAS_COMMAND_KEEP_SPACES weechat_hook_set (ptr_hook, "keep_spaces_right", "1")

extern void alias_command_init (void);

#endif /* WEECHAT_PLUGIN_ALIAS_COMMAND_H */
