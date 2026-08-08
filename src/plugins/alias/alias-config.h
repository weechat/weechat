/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_ALIAS_CONFIG_H
#define WEECHAT_PLUGIN_ALIAS_CONFIG_H

#define ALIAS_CONFIG_NAME "alias"
#define ALIAS_CONFIG_PRIO_NAME (TO_STR(ALIAS_PLUGIN_PRIORITY) "|" ALIAS_CONFIG_NAME)

#define ALIAS_CONFIG_VERSION 2

extern struct t_config_file *alias_config_file;
extern struct t_config_section *alias_config_section_cmd;
extern struct t_config_section *alias_config_section_completion;

extern char *alias_default[][3];

extern void alias_config_cmd_new_option (const char *name,
                                         const char *command);
extern void alias_config_completion_new_option (const char *name,
                                                const char *completion);
extern int alias_config_init (void);
extern int alias_config_read (void);
extern int alias_config_write (void);

#endif /* WEECHAT_PLUGIN_ALIAS_CONFIG_H */
