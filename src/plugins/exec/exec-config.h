/*
 * Copyright (C) 2014 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef __WEECHAT_EXEC_CONFIG_H
#define __WEECHAT_EXEC_CONFIG_H 1

#define EXEC_CONFIG_NAME "exec"
#define EXEC_CONFIG_SECTION_EXEC "exec"

extern struct t_config_file *exec_config_file;

extern struct t_config_option *exec_config_command_purge_delay;

extern struct t_config_option *exec_config_color_flag_running;
extern struct t_config_option *exec_config_color_flag_finished;

extern int exec_config_init ();
extern int exec_config_read ();
extern int exec_config_write ();
extern void exec_config_free ();

#endif /* __WEECHAT_EXEC_CONFIG_H */
