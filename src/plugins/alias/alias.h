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

#define ALIAS_CONFIG_FILENAME "alias.rc"

struct t_alias
{
    struct t_hook *hook;               /* command hook                      */
    char *name;                        /* alias name                        */
    char *command;                     /* alias command                     */
    int running;                       /* 1 if alias is running             */
    struct t_alias *prev_alias;        /* link to previous alias            */
    struct t_alias *next_alias;        /* link to next alias                */
};

#endif /* alias.h */
