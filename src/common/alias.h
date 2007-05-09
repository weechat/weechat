/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef __WEECHAT_ALIAS_H
#define __WEECHAT_ALIAS_H 1

#include "../irc/irc.h"

typedef struct t_weechat_alias t_weechat_alias;

struct t_weechat_alias
{
    char *alias_name;
    char *alias_command;
    int running;
    t_weechat_alias *prev_alias;
    t_weechat_alias *next_alias;
};

extern t_weechat_alias *weechat_alias;
extern t_weechat_alias *weechat_last_alias;

extern t_weechat_alias *alias_search (char *);
extern t_weechat_alias *alias_new (char *, char *);
extern char *alias_get_final_command (t_weechat_alias *);
extern char *alias_replace_args (char *, char *);
extern char *alias_replace_vars (t_irc_server *, t_irc_channel *, char *);
extern void alias_free (t_weechat_alias *);
extern void alias_free_all ();

#endif /* alias.h */
