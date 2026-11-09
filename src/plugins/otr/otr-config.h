/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_CONFIG_H
#define WEECHAT_PLUGIN_OTR_CONFIG_H

enum t_otr_config_color
{
    OTR_CONFIG_COLOR_STATUS_DEFAULT = 0,
    OTR_CONFIG_COLOR_STATUS_ENCRYPTED,
    OTR_CONFIG_COLOR_STATUS_UNENCRYPTED,
    OTR_CONFIG_COLOR_STATUS_AUTHENTICATED,
    OTR_CONFIG_COLOR_STATUS_UNAUTHENTICATED,
    OTR_CONFIG_COLOR_STATUS_LOGGED,
    OTR_CONFIG_COLOR_STATUS_NOTLOGGED,
    OTR_CONFIG_COLOR_BUFFER_HINT,
    OTR_CONFIG_COLOR_BUFFER_INFO,
    OTR_CONFIG_COLOR_BUFFER_SUCCESS,
    OTR_CONFIG_COLOR_BUFFER_WARNING,
    OTR_CONFIG_COLOR_BUFFER_ERROR,
    /* number of color options */
    OTR_CONFIG_NUM_COLOR_OPTIONS,
};

#define OTR_CONFIG_LEVEL_HINT    OTR_CONFIG_COLOR_BUFFER_HINT
#define OTR_CONFIG_LEVEL_INFO    OTR_CONFIG_COLOR_BUFFER_INFO
#define OTR_CONFIG_LEVEL_SUCCESS OTR_CONFIG_COLOR_BUFFER_SUCCESS
#define OTR_CONFIG_LEVEL_WARNING OTR_CONFIG_COLOR_BUFFER_WARNING
#define OTR_CONFIG_LEVEL_ERROR   OTR_CONFIG_COLOR_BUFFER_ERROR

enum t_otr_config_look
{
    OTR_CONFIG_LOOK_BAR_PREFIX = 0,
    OTR_CONFIG_LOOK_BAR_ENCRYPTED,
    OTR_CONFIG_LOOK_BAR_UNENCRYPTED,
    OTR_CONFIG_LOOK_BAR_AUTHENTICATED,
    OTR_CONFIG_LOOK_BAR_UNAUTHENTICATED,
    OTR_CONFIG_LOOK_BAR_LOGGED,
    OTR_CONFIG_LOOK_BAR_NOTLOGGED,
    OTR_CONFIG_LOOK_BAR_SEPARATOR,
    OTR_CONFIG_LOOK_PREFIX,
    /* number of look options */
    OTR_CONFIG_NUM_LOOK_OPTIONS,
};

struct t_otr_policy
{
    const char *name, *desc;
};

#define OTR_CONFIG_NUM_POLICIES otr_config_policies_count

extern const struct t_otr_policy otr_config_policies[];
extern const int otr_config_policies_count;

extern struct t_config_file *otr_config_file;
extern struct t_config_section *otr_config_section_policy;
extern struct t_config_option *otr_config_general_debug;
extern struct t_config_option *otr_config_general_hints;
extern struct t_config_option *otr_config_general_no_send_tag_regex;
extern struct t_config_option *otr_config_color[OTR_CONFIG_NUM_COLOR_OPTIONS];
extern struct t_config_option *otr_config_look[OTR_CONFIG_NUM_LOOK_OPTIONS];

extern int otr_config_init (void);

#endif /* WEECHAT_PLUGIN_OTR_CONFIG_H */
