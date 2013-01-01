/*
 * Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2006-2013 Sebastien Helleu <flashcode@flashtux.org>
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
extern int aspell_enabled;

extern void weechat_aspell_create_spellers (struct t_gui_buffer *buffer);
extern const char *weechat_aspell_get_dict (struct t_gui_buffer *buffer);

#endif /* __WEECHAT_ASPELL_H */
