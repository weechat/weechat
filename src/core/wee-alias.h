/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_ALIAS_H
#define __WEECHAT_ALIAS_H 1

#include "../gui/gui-buffer.h"

struct alias
{
    char *name;
    char *command;
    int running;
    struct alias *prev_alias;
    struct alias *next_alias;
};

extern struct alias *weechat_alias;
extern struct alias *weechat_last_alias;

extern struct alias *alias_search (char *);
extern struct alias *alias_new (char *, char *);
extern char *alias_get_final_command (struct alias *);
extern char *alias_replace_args (char *, char *);
extern char *alias_replace_vars (struct t_gui_buffer *, char *);
extern void alias_free (struct alias *);
extern void alias_free_all ();

#endif /* wee-alias.h */
