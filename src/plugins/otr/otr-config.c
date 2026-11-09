/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* OTR configuration options (file otr.conf) */

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-config.h"


static const struct
{
    const char *name, *desc, *def;
    int update_bar;
} otr_config_color_defs[OTR_CONFIG_NUM_COLOR_OPTIONS] = {
    { "status.default", N_("status bar default color"), "default", 1 },
    { "status.encrypted", N_("status bar encrypted indicator color"), "green", 1 },
    { "status.unencrypted", N_("status bar unencrypted indicator color"),
      "lightred", 1 },
    { "status.authenticated", N_("status bar authenticated indicator color"),
      "green", 1 },
    { "status.unauthenticated", N_("status bar unauthenticated indicator color"),
      "lightred", 1 },
    { "status.logged", N_("status bar logged indicator color"), "lightred", 1 },
    { "status.notlogged", N_("status bar not logged indicator color"), "green",
      1 },
    { "buffer.hint", N_("text color for hints"), "lightblue", 0 },
    { "buffer.info", N_("text color for informational messages"), "default", 0 },
    { "buffer.success", N_("text color for success messages"), "lightgreen", 0 },
    { "buffer.warning", N_("text color for warnings"), "yellow", 0 },
    { "buffer.error", N_("text color for errors"), "lightred", 0 },
};

static const struct
{
    const char *name, *desc, *def;
    int update_bar;
} otr_config_look_defs[OTR_CONFIG_NUM_LOOK_OPTIONS] = {
    { "bar.prefix", N_("prefix for OTR status bar item"), "OTR:", 1 },
    { "bar.state.encrypted",
      N_("shown in status bar when conversation is encrypted"), "SEC", 1 },
    { "bar.state.unencrypted",
      N_("shown in status bar when conversation is not encrypted"), "!SEC", 1 },
    { "bar.state.authenticated",
      N_("shown in status bar when peer is authenticated"), "AUTH", 1 },
    { "bar.state.unauthenticated",
      N_("shown in status bar when peer is not authenticated"), "!AUTH", 1 },
    { "bar.state.logged",
      N_("shown in status bar when peer conversation is being logged to disk"),
      "LOG", 1 },
    { "bar.state.notlogged",
      N_("shown in status bar when peer conversation is not being logged to "
      "disk"), "!LOG", 1 },
    { "bar.state.separator", N_("separator for states in the status bar"), ",",
      1 },
    { "prefix", N_("prefix used for messages from otr (note: content is "
      "evaluated, see /help eval)"),
      "${color:default}:! ${color:brown}otr${color:default} !:", 0 },
};


const struct t_otr_policy otr_config_policies[] = {
    { "allow_v2", N_("allow OTR protocol version 2") },
    { "allow_v3", N_("allow OTR protocol version 3") },
    { "require_encryption", N_("refuse to send unencrypted messages when OTR is "
      "enabled") },
    { "log", N_("enable logging of OTR conversations") },
    { "send_tag", N_("advertise your OTR capability using the whitespace tag") },
    { "html_escape", N_("escape HTML special characters in outbound messages") },
    { "html_filter", N_("filter HTML in incoming messages") },
};
const int otr_config_policies_count =
    (int)(sizeof (otr_config_policies) / sizeof (otr_config_policies[0]));

static const struct
{
    const char *name, *desc, *def;
} otr_config_policy_defaults[] = {
    { "default.allow_v2", N_("default allow OTR v2 policy"), "on" },
    { "default.allow_v3", N_("default allow OTR v3 policy"), "on" },
    { "default.require_encryption", N_("default require encryption policy"),
      "off" },
    { "default.log", N_("default enable logging to disk"), "off" },
    { "default.send_tag", N_("default send tag policy"), "off" },
    { "default.html_escape", N_("default HTML escape policy"), "off" },
    { "default.html_filter", N_("default HTML filter policy"), "on" },
};

struct t_config_file *otr_config_file = NULL;
static struct t_config_section *otr_config_section_general = NULL;
static struct t_config_section *otr_config_section_color = NULL;
static struct t_config_section *otr_config_section_look = NULL;
struct t_config_section *otr_config_section_policy = NULL;
struct t_config_option *otr_config_general_debug = NULL;
struct t_config_option *otr_config_general_hints = NULL;
struct t_config_option *otr_config_general_no_send_tag_regex = NULL;
struct t_config_option *otr_config_color[OTR_CONFIG_NUM_COLOR_OPTIONS];
struct t_config_option *otr_config_look[OTR_CONFIG_NUM_LOOK_OPTIONS];

static void
otr_config_change_bar_cb (const void *pointer, void *data,
                          struct t_config_option *option)
{
    (void)pointer; (void)data; (void)option;
    weechat_bar_item_update (OTR_BAR_ITEM);
}

static int
otr_config_policy_create_option_cb (const void *pointer, void *data,
                                    struct t_config_file *config_file,
                                    struct t_config_section *section,
                                    const char *option_name, const char *value)
{
    struct t_config_option *o;

    (void)pointer; (void)data;
    if (!value)
        value = "off";
    o = weechat_config_new_option (config_file, section, option_name,
                                   "boolean", "", NULL, 0, 0, value, value, 0,
                                   NULL, NULL, NULL,
                                   NULL, NULL, NULL,
                                   NULL, NULL, NULL);
    return o ? WEECHAT_CONFIG_OPTION_SET_OK_CHANGED
             : WEECHAT_CONFIG_OPTION_SET_ERROR;
}

static struct t_config_option *
otr_config_new_option (struct t_config_section *sec, const char *name,
                       const char *type,
                       const char *desc, const char *def,
                       void (*change_cb)(const void *, void *,
                                         struct t_config_option *))
{
    return weechat_config_new_option (otr_config_file, sec, name, type, desc,
                                      NULL,
                                      0, 0, def, def, 0,
                                      NULL, NULL, NULL,
                                      change_cb, NULL, NULL,
                                      NULL, NULL, NULL);
}

int
otr_config_init (void)
{
    int i;

    otr_config_file = weechat_config_new (OTR_CONFIG_PRIO_NAME, NULL, NULL,
                                          NULL);
    if (!otr_config_file)
        return 0;

    otr_config_section_general = weechat_config_new_section (
        otr_config_file, "general", 0, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL);
    otr_config_section_color = weechat_config_new_section (
        otr_config_file, "color", 0, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL);
    otr_config_section_look = weechat_config_new_section (
        otr_config_file, "look", 0, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL);
    otr_config_section_policy = weechat_config_new_section (
        otr_config_file, "policy", 1, 1,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        otr_config_policy_create_option_cb, NULL, NULL, NULL, NULL, NULL);
    if (!otr_config_section_general || !otr_config_section_color
        || !otr_config_section_look
        || !otr_config_section_policy)
        return 0;

    otr_config_general_debug = otr_config_new_option (
        otr_config_section_general, "debug", "boolean",
        N_("OTR plugin debugging (metadata only, never message contents)"),
        "off", NULL);
    otr_config_general_hints = otr_config_new_option (
        otr_config_section_general, "hints", "boolean",
        N_("Give helpful hints how to use this plugin and how "
        "to stay secure while using OTR (recommended)"),
        "on", NULL);
    otr_config_general_no_send_tag_regex = otr_config_new_option (
        otr_config_section_general, "no_send_tag_regex", "string",
        N_("do not OTR whitespace tag messages to nicks matching this regex "
        "(POSIX extended, case insensitive)"),
        "^(alis|chanfix|global|.+serv|\\*.+)$", NULL);

    for (i = 0; i < OTR_CONFIG_NUM_COLOR_OPTIONS; i++)
        otr_config_color[i] = otr_config_new_option (
            otr_config_section_color, otr_config_color_defs[i].name, "color",
            otr_config_color_defs[i].desc, otr_config_color_defs[i].def,
            otr_config_color_defs[i].update_bar
            ? otr_config_change_bar_cb : NULL);
    for (i = 0; i < OTR_CONFIG_NUM_LOOK_OPTIONS; i++)
        otr_config_look[i] = otr_config_new_option (
            otr_config_section_look, otr_config_look_defs[i].name, "string",
            otr_config_look_defs[i].desc, otr_config_look_defs[i].def,
            otr_config_look_defs[i].update_bar
            ? otr_config_change_bar_cb : NULL);
    for (i = 0;
         i < (int)(sizeof (otr_config_policy_defaults)
                   / sizeof (otr_config_policy_defaults[0]));
         i++)
        otr_config_new_option (otr_config_section_policy,
                               otr_config_policy_defaults[i].name, "boolean",
                               otr_config_policy_defaults[i].desc,
                               otr_config_policy_defaults[i].def, NULL);

    weechat_config_read (otr_config_file);
    return 1;
}
