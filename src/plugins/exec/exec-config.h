/*
 * SPDX-FileCopyrightText: 2014-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_EXEC_CONFIG_H
#define WEECHAT_PLUGIN_EXEC_CONFIG_H

#define EXEC_CONFIG_NAME "exec"
#define EXEC_CONFIG_PRIO_NAME (TO_STR(EXEC_PLUGIN_PRIORITY) "|" EXEC_CONFIG_NAME)

extern struct t_config_file *exec_config_file;

extern struct t_config_option *exec_config_command_default_options;
extern struct t_config_option *exec_config_command_purge_delay;
extern struct t_config_option *exec_config_command_shell;

extern struct t_config_option *exec_config_color_flag_finished;
extern struct t_config_option *exec_config_color_flag_running;

extern char **exec_config_cmd_options;
extern int exec_config_cmd_num_options;

extern int exec_config_init (void);
extern int exec_config_read (void);
extern int exec_config_write (void);
extern void exec_config_free (void);

#endif /* WEECHAT_PLUGIN_EXEC_CONFIG_H */
