/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
