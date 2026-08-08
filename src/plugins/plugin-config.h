/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_PLUGIN_CONFIG_H
#define WEECHAT_PLUGIN_PLUGIN_CONFIG_H

#define PLUGIN_CONFIG_NAME "plugins"
#define PLUGIN_CONFIG_PRIO_NAME "100000|plugins"

extern struct t_config_file *plugin_config_file;
extern struct t_config_option *plugin_options;

extern struct t_config_option *plugin_config_search (const char *plugin_name,
                                                     const char *option_name);
extern int plugin_config_set (const char *plugin_name, const char *option_name,
                              const char *value);
extern void plugin_config_set_desc (const char *plugin_name,
                                    const char *option_name,
                                    const char *description);
extern void plugin_config_init (void);
extern int plugin_config_read (void);
extern int plugin_config_write (void);
extern void plugin_config_end (void);

#endif /* WEECHAT_PLUGIN_PLUGIN_CONFIG_H */
