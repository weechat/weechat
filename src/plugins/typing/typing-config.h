/*
 * SPDX-FileCopyrightText: 2021-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_TYPING_CONFIG_H
#define WEECHAT_PLUGIN_TYPING_CONFIG_H

#define TYPING_CONFIG_NAME "typing"
#define TYPING_CONFIG_PRIO_NAME (TO_STR(TYPING_PLUGIN_PRIORITY) "|" TYPING_CONFIG_NAME)

extern struct t_config_option *typing_config_look_delay_purge_paused;
extern struct t_config_option *typing_config_look_delay_purge_typing;
extern struct t_config_option *typing_config_look_delay_set_paused;
extern struct t_config_option *typing_config_look_enabled_nicks;
extern struct t_config_option *typing_config_look_enabled_self;
extern struct t_config_option *typing_config_look_input_min_chars;
extern struct t_config_option *typing_config_look_item_max_length;
extern struct t_config_option *typing_config_look_item_text;

extern int typing_config_init (void);
extern int typing_config_read (void);
extern int typing_config_write (void);
extern void typing_config_free (void);

#endif /* WEECHAT_PLUGIN_TYPING_CONFIG_H */
