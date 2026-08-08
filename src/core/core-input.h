/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_INPUT_H
#define WEECHAT_INPUT_H

struct t_gui_buffer;
struct t_weechat_plugin;

extern char **input_commands_allowed;

extern int input_exec_command (struct t_gui_buffer *buffer,
                               int any_plugin,
                               struct t_weechat_plugin *plugin,
                               const char *string,
                               const char *commands_allowed);
extern int input_data (struct t_gui_buffer *buffer, const char *data,
                       const char *commands_allowed, int split_newline,
                       int user_data);
extern int input_data_delayed (struct t_gui_buffer *buffer, const char *data,
                               const char *commands_allowed, int split_newline,
                               long delay);

#endif /* WEECHAT_INPUT_H */
