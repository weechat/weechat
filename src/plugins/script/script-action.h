/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_SCRIPT_ACTION_H
#define WEECHAT_PLUGIN_SCRIPT_ACTION_H

extern char **script_actions;

extern int script_action_run_all (void);
extern void script_action_schedule (struct t_gui_buffer *buffer,
                                    const char *action,
                                    int need_repository,
                                    int error_repository,
                                    int quiet);
extern void script_action_end (void);

#endif /* WEECHAT_PLUGIN_SCRIPT_ACTION_H */
