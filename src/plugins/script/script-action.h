/*
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_SCRIPT_ACTION_H
#define WEECHAT_PLUGIN_SCRIPT_ACTION_H

extern char **script_actions;

extern int script_action_run_all ();
extern void script_action_schedule (struct t_gui_buffer *buffer,
                                    const char *action,
                                    int need_repository,
                                    int error_repository,
                                    int quiet);
extern void script_action_end ();

#endif /* WEECHAT_PLUGIN_SCRIPT_ACTION_H */
