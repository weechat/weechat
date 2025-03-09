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

#ifndef WEECHAT_PLUGIN_FSET_H
#define WEECHAT_PLUGIN_FSET_H

#define weechat_plugin weechat_fset_plugin
#define FSET_PLUGIN_NAME "fset"
#define FSET_PLUGIN_PRIORITY 2000

#define FSET_BAR_NAME "fset"

extern struct t_weechat_plugin *weechat_fset_plugin;

extern struct t_hdata *fset_hdata_config_file;
extern struct t_hdata *fset_hdata_config_section;
extern struct t_hdata *fset_hdata_config_option;
extern struct t_hdata *fset_hdata_fset_option;

extern void fset_add_bar (void);

#endif /* WEECHAT_PLUGIN_FSET_H */
