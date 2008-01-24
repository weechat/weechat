/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_CHARSET_H
#define __WEECHAT_CHARSET_H 1

#define CHARSET_CONFIG_FILENAME "charset.rc"

struct t_charset
{
    char *name;                        /* charset name (identifier)         */
    char *charset;                     /* charset value for name            */
    struct t_charset *prev_charset;    /* link to previous charset          */
    struct t_charset *next_charset;    /* link to next charset              */
};

#endif /* charset.h */
