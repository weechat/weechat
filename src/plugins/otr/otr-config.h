/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_CONFIG_H
#define WEECHAT_PLUGIN_OTR_CONFIG_H

enum
{
    COL_STATUS_DEFAULT, COL_STATUS_ENCRYPTED, COL_STATUS_UNENCRYPTED,
    COL_STATUS_AUTHENTICATED, COL_STATUS_UNAUTHENTICATED,
    COL_STATUS_LOGGED, COL_STATUS_NOTLOGGED,
    COL_BUF_HINT, COL_BUF_INFO, COL_BUF_SUCCESS, COL_BUF_WARNING,
    COL_BUF_ERROR,
    COL_COUNT
};

#define LVL_HINT    COL_BUF_HINT
#define LVL_INFO    COL_BUF_INFO
#define LVL_SUCCESS COL_BUF_SUCCESS
#define LVL_WARNING COL_BUF_WARNING
#define LVL_ERROR   COL_BUF_ERROR

enum
{
    LOOK_BAR_PREFIX, LOOK_BAR_ENCRYPTED, LOOK_BAR_UNENCRYPTED,
    LOOK_BAR_AUTHENTICATED, LOOK_BAR_UNAUTHENTICATED, LOOK_BAR_LOGGED,
    LOOK_BAR_NOTLOGGED, LOOK_BAR_SEPARATOR, LOOK_PREFIX,
    LOOK_COUNT
};

struct t_otr_policy
{
    const char *name, *desc;
};

#define N_POLICIES policies_count

extern const struct t_otr_policy policies[];
extern const int policies_count;

extern struct t_config_file *cfg_file;
extern struct t_config_section *cfg_sec_policy;
extern struct t_config_option *opt_debug;
extern struct t_config_option *opt_hints;
extern struct t_config_option *opt_no_send_tag_regex;
extern struct t_config_option *opt_color[COL_COUNT];
extern struct t_config_option *opt_look[LOOK_COUNT];

extern int config_init (void);

#endif /* WEECHAT_PLUGIN_OTR_CONFIG_H */
