/*
 * Copyright (c) 2003-2010 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __WEECHAT_COMMAND_H
#define __WEECHAT_COMMAND_H 1

/*
 * This macro is used to create an "empty" command in WeeChat core:
 * command does nothing, but plugins or scripts can catch it when it
 * is used by user, with weechat_hook_command_run("/xxx", ...)
 * where "xxx" is command name.
 */
#define COMMAND_EMPTY(__command)                                        \
    int                                                                 \
    command_##__command (void *data, struct t_gui_buffer *buffer,       \
                         int argc, char **argv, char **argv_eol)        \
    {                                                                   \
        (void) data;                                                    \
        (void) buffer;                                                  \
        (void) argc;                                                    \
        (void) argv;                                                    \
        (void) argv_eol;                                                \
        return WEECHAT_RC_OK;                                           \
    }

struct t_gui_buffer;

extern int command_reload (void *data, struct t_gui_buffer *buffer,
                           int argc, char **argv, char **argv_eol);
extern void command_init ();
extern void command_startup (int plugins_looaded);
extern void command_version_display (struct t_gui_buffer *buffer,
                                     int send_to_buffer_as_input,
                                     int translated_string);

#endif /* wee-command.h */
