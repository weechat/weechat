/*
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_HOOK_COMMAND_RUN_H
#define WEECHAT_HOOK_COMMAND_RUN_H

struct t_weechat_plugin;
struct t_infolist_item;
struct t_gui_buffer;

#define HOOK_COMMAND_RUN(hook, var) (((struct t_hook_command_run *)hook->hook_data)->var)

typedef int (t_hook_callback_command_run)(const void *pointer, void *data,
                                          struct t_gui_buffer *buffer,
                                          const char *command);

struct t_hook_command_run
{
    t_hook_callback_command_run *callback; /* command_run callback          */
    char *command;                     /* name of command (without '/')     */

    int keep_spaces_right;             /* if set to 1: don't strip trailing */
                                       /* spaces in args when the command   */
                                       /* is executed                       */
};

extern char *hook_command_run_get_description (struct t_hook *hook);
extern struct t_hook *hook_command_run (struct t_weechat_plugin *plugin,
                                        const char *command,
                                        t_hook_callback_command_run *callback,
                                        const void *callback_pointer,
                                        void *callback_data);
extern int hook_command_run_exec (struct t_gui_buffer *buffer,
                                  const char *command);
extern void hook_command_run_free_data (struct t_hook *hook);
extern int hook_command_run_add_to_infolist (struct t_infolist_item *item,
                                             struct t_hook *hook);
extern void hook_command_run_print_log (struct t_hook *hook);

#endif /* WEECHAT_HOOK_COMMAND_RUN_H */
