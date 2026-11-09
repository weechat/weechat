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
} color_defs[COL_COUNT] = {
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
} look_defs[LOOK_COUNT] = {
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


const struct t_otr_policy policies[] = {
    { "allow_v2", N_("allow OTR protocol version 2") },
    { "allow_v3", N_("allow OTR protocol version 3") },
    { "require_encryption", N_("refuse to send unencrypted messages when OTR is "
      "enabled") },
    { "log", N_("enable logging of OTR conversations") },
    { "send_tag", N_("advertise your OTR capability using the whitespace tag") },
    { "html_escape", N_("escape HTML special characters in outbound messages") },
    { "html_filter", N_("filter HTML in incoming messages") },
};
const int policies_count = (int)(sizeof (policies) / sizeof (policies[0]));

static const struct
{
    const char *name, *desc, *def;
} policy_defaults[] = {
    { "default.allow_v2", N_("default allow OTR v2 policy"), "on" },
    { "default.allow_v3", N_("default allow OTR v3 policy"), "on" },
    { "default.require_encryption", N_("default require encryption policy"),
      "off" },
    { "default.log", N_("default enable logging to disk"), "off" },
    { "default.send_tag", N_("default send tag policy"), "off" },
    { "default.html_escape", N_("default HTML escape policy"), "off" },
    { "default.html_filter", N_("default HTML filter policy"), "on" },
};

struct t_config_file *cfg_file = NULL;
static struct t_config_section *cfg_sec_general = NULL;
static struct t_config_section *cfg_sec_color = NULL;
static struct t_config_section *cfg_sec_look = NULL;
struct t_config_section *cfg_sec_policy = NULL;
struct t_config_option *opt_debug = NULL;
struct t_config_option *opt_hints = NULL;
struct t_config_option *opt_no_send_tag_regex = NULL;
struct t_config_option *opt_color[COL_COUNT];
struct t_config_option *opt_look[LOOK_COUNT];

static void
config_change_bar_cb (const void *pointer, void *data,
                      struct t_config_option *option)
{
    (void)pointer; (void)data; (void)option;
    weechat_bar_item_update (OTR_BAR_ITEM);
}

static int
config_policy_create_option_cb (const void *pointer, void *data,
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
new_opt (struct t_config_section *sec, const char *name, const char *type,
         const char *desc, const char *def,
         void (*change_cb)(const void *, void *, struct t_config_option *))
{
    return weechat_config_new_option (cfg_file, sec, name, type, desc, NULL,
                                      0, 0, def, def, 0,
                                      NULL, NULL, NULL,
                                      change_cb, NULL, NULL,
                                      NULL, NULL, NULL);
}

int
config_init (void)
{
    int i;

    cfg_file = weechat_config_new ("otr", NULL, NULL, NULL);
    if (!cfg_file)
        return 0;

    cfg_sec_general = weechat_config_new_section (
        cfg_file, "general", 0, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL);
    cfg_sec_color = weechat_config_new_section (
        cfg_file, "color", 0, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL);
    cfg_sec_look = weechat_config_new_section (
        cfg_file, "look", 0, 0,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL);
    cfg_sec_policy = weechat_config_new_section (
        cfg_file, "policy", 1, 1,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        config_policy_create_option_cb, NULL, NULL, NULL, NULL, NULL);
    if (!cfg_sec_general || !cfg_sec_color || !cfg_sec_look
        || !cfg_sec_policy)
        return 0;

    opt_debug = new_opt (cfg_sec_general, "debug", "boolean",
                         N_("OTR plugin debugging (metadata only, never message "
                         "contents)"), "off", NULL);
    opt_hints = new_opt (cfg_sec_general, "hints", "boolean",
                         N_("Give helpful hints how to use this plugin and how "
                         "to stay secure while using OTR (recommended)"),
                         "on", NULL);
    opt_no_send_tag_regex = new_opt (
        cfg_sec_general, "no_send_tag_regex", "string",
        N_("do not OTR whitespace tag messages to nicks matching this regex "
        "(POSIX extended, case insensitive)"),
        "^(alis|chanfix|global|.+serv|\\*.+)$", NULL);

    for (i = 0; i < COL_COUNT; i++)
        opt_color[i] = new_opt (cfg_sec_color, color_defs[i].name, "color",
                                color_defs[i].desc, color_defs[i].def,
                                color_defs[i].update_bar
                                ? config_change_bar_cb : NULL);
    for (i = 0; i < LOOK_COUNT; i++)
        opt_look[i] = new_opt (cfg_sec_look, look_defs[i].name, "string",
                               look_defs[i].desc, look_defs[i].def,
                               look_defs[i].update_bar
                               ? config_change_bar_cb : NULL);
    for (i = 0; i < (int)(sizeof (policy_defaults) / sizeof (policy_defaults[0]));
         i++)
        new_opt (cfg_sec_policy, policy_defaults[i].name, "boolean",
                 policy_defaults[i].desc, policy_defaults[i].def, NULL);

    weechat_config_read (cfg_file);
    return 1;
}
