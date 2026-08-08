/*
 * SPDX-FileCopyrightText: 2014-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_TRIGGER_CONFIG_H
#define WEECHAT_PLUGIN_TRIGGER_CONFIG_H

#define TRIGGER_CONFIG_NAME "trigger"
#define TRIGGER_CONFIG_PRIO_NAME (TO_STR(TRIGGER_PLUGIN_PRIORITY) "|" TRIGGER_CONFIG_NAME)
#define TRIGGER_CONFIG_SECTION_TRIGGER "trigger"

extern struct t_config_file *trigger_config_file;

extern struct t_config_section *trigger_config_section_look;
extern struct t_config_section *trigger_config_section_color;
extern struct t_config_section *trigger_config_section_trigger;

extern struct t_config_option *trigger_config_look_enabled;
extern struct t_config_option *trigger_config_look_monitor_strip_colors;

extern struct t_config_option *trigger_config_color_flag_command;
extern struct t_config_option *trigger_config_color_flag_conditions;
extern struct t_config_option *trigger_config_color_flag_regex;
extern struct t_config_option *trigger_config_color_flag_return_code;
extern struct t_config_option *trigger_config_color_flag_post_action;
extern struct t_config_option *trigger_config_color_identifier;
extern struct t_config_option *trigger_config_color_regex;
extern struct t_config_option *trigger_config_color_replace;

extern char *trigger_config_default_list[][1 + TRIGGER_NUM_OPTIONS];

extern struct t_config_option *trigger_config_create_trigger_option (const char *trigger_name,
                                                                     int index_option,
                                                                     const char *value);
extern int trigger_config_init (void);
extern int trigger_config_read (void);
extern int trigger_config_write (void);
extern void trigger_config_free (void);

#endif /* WEECHAT_PLUGIN_TRIGGER_CONFIG_H */
