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


#ifndef __WEECHAT_ASPELL_H
#define __WEECHAT_ASPELL_H 1

#include <aspell.h>

#define weechat_plugin weechat_aspell_plugin
#define ASPELL_PLUGIN_NAME "aspell"

struct t_aspell_code
{
    char *code;
    char *name;
};

extern struct t_weechat_plugin *weechat_aspell_plugin;

extern struct t_aspell_code langs_avail[];
extern struct t_aspell_code countries_avail[];

extern void weechat_aspell_create_spellers (struct t_gui_buffer *buffer);

#endif /* aspell.h */
