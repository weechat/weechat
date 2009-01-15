/*
 * Copyright (c) 2009 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_JABBER_COMMAND_H
#define __WEECHAT_JABBER_COMMAND_H 1

struct t_jabber_server;

#define JABBER_COMMAND_TOO_FEW_ARGUMENTS(__buffer, __command)              \
    weechat_printf (__buffer,                                              \
                    _("%s%s: too few arguments for \"%s\" command"),       \
                    jabber_buffer_get_server_prefix (ptr_server, "error"), \
                    JABBER_PLUGIN_NAME,                                    \
                    __command);                                            \
    return WEECHAT_RC_ERROR;

extern void jabber_command_quit_server (struct t_jabber_server *server,
                                        const char *arguments);
extern void jabber_command_init ();

#endif /* jabber-command.h */
