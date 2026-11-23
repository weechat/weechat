/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* OTR configuration options (file otr.conf) */

#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-config.h"


struct t_config_file *otr_config_file = NULL;

/* OTR config, sections */

struct t_config_section *otr_config_section_color = NULL;
struct t_config_section *otr_config_section_look = NULL;
struct t_config_section *otr_config_section_policy_default = NULL;
struct t_config_section *otr_config_section_policy = NULL;

/* OTR config, look section (options not in otr_config_look) */

struct t_config_option *otr_config_look_debug = NULL;
struct t_config_option *otr_config_look_hints = NULL;
struct t_config_option *otr_config_look_no_send_tag_regex = NULL;

/* Compiled regex of option otr.look.no_send_tag_regex */

regex_t *otr_config_regex_no_send_tag = NULL;

/* OTR config, color and look sections */

struct t_config_option *otr_config_color[OTR_CONFIG_NUM_COLOR_OPTIONS];
struct t_config_option *otr_config_look[OTR_CONFIG_NUM_LOOK_OPTIONS];

/* Definition of color options (same order as enum t_otr_config_color) */

const struct t_otr_config_option_def
    otr_config_color_defs[OTR_CONFIG_NUM_COLOR_OPTIONS] =
{
    { "status_default",
      N_("default text color for bar item \"otr\""),
      "default", 1 },
    { "status_encrypted",
      N_("text color for encrypted indicator in status bar"),
      "green", 1 },
    { "status_unencrypted",
      N_("text color for unencrypted indicator in status bar"),
      "lightred", 1 },
    { "status_authenticated",
      N_("text color for authenticated indicator in status bar"),
      "green", 1 },
    { "status_unauthenticated",
      N_("text color for unauthenticated indicator in status bar"),
      "lightred", 1 },
    { "status_logged",
      N_("text color for logged indicator in status bar"),
      "lightred", 1 },
    { "status_notlogged",
      N_("text color for not logged indicator in status bar"),
      "green", 1 },
    { "buffer_hint", N_("text color for hints"), "lightblue", 0 },
    { "buffer_info", N_("text color for informational messages"),
      "default", 0 },
    { "buffer_success", N_("text color for success messages"),
      "lightgreen", 0 },
    { "buffer_warning", N_("text color for warnings"), "yellow", 0 },
    { "buffer_error", N_("text color for errors"), "lightred", 0 },
};

/* Definition of look options (same order as enum t_otr_config_look) */

const struct t_otr_config_option_def
    otr_config_look_defs[OTR_CONFIG_NUM_LOOK_OPTIONS] =
{
    { "bar_prefix", N_("prefix for bar item \"otr\""), "OTR:", 1 },
    { "bar_state_encrypted",
      N_("shown in status bar when the conversation is encrypted"),
      "SEC", 1 },
    { "bar_state_unencrypted",
      N_("shown in status bar when the conversation is not encrypted"),
      "!SEC", 1 },
    { "bar_state_authenticated",
      N_("shown in status bar when the peer is authenticated"),
      "AUTH", 1 },
    { "bar_state_unauthenticated",
      N_("shown in status bar when the peer is not authenticated"),
      "!AUTH", 1 },
    { "bar_state_logged",
      N_("shown in status bar when the conversation is being logged to disk"),
      "LOG", 1 },
    { "bar_state_notlogged",
      N_("shown in status bar when the conversation is not being logged to "
         "disk"),
      "!LOG", 1 },
    { "bar_state_separator", N_("separator for states in status bar"),
      ",", 1 },
    { "prefix",
      N_("prefix used for messages from OTR (note: content is evaluated, see "
         "/help eval)"),
      "${color:default}=${color:brown}otr${color:default}=", 0 },
};

/* OTR policies */

const struct t_otr_policy otr_config_policies[] =
{
    { "allow_v2", N_("allow version 2 of the OTR protocol") },
    { "allow_v3", N_("allow version 3 of the OTR protocol") },
    { "require_encryption",
      N_("refuse to send unencrypted messages: messages sent before the OTR "
         "session is established are held, and sent once the session is "
         "encrypted (CTCP messages other than ACTION are still sent "
         "unencrypted)") },
    { "log",
      N_("keep logging enabled during OTR sessions (if off, logging is "
         "disabled for the session and restored when it ends)") },
    { "send_tag",
      N_("advertise your OTR support with a whitespace tag added to your "
         "messages, so that an OTR session can start automatically") },
    { "html_escape",
      N_("escape HTML special chars in encrypted messages sent") },
    { "html_filter", N_("remove HTML in decrypted messages received") },
};
const int otr_config_policies_count =
    (int)(sizeof (otr_config_policies) / sizeof (otr_config_policies[0]));

/* Definition of default policies options (section "policy_default") */

const struct t_otr_config_option_def otr_config_policy_defaults[] =
{
    { "allow_v2",
      N_("allow version 2 of the OTR protocol (default value, options "
         "otr.policy.* can override this value)"),
      "on", 0 },
    { "allow_v3",
      N_("allow version 3 of the OTR protocol (default value, options "
         "otr.policy.* can override this value)"),
      "on", 0 },
    { "require_encryption",
      N_("refuse to send unencrypted messages: messages sent before the OTR "
         "session is established are held, and sent once the session is "
         "encrypted (CTCP messages other than ACTION are still sent "
         "unencrypted) (default value, options otr.policy.* can override this "
         "value)"),
      "off", 0 },
    { "log",
      N_("keep logging enabled during OTR sessions (if off, logging is "
         "disabled for the session and restored when it ends) (default value, "
         "options otr.policy.* can override this value)"),
      "off", 0 },
    { "send_tag",
      N_("advertise your OTR support with a whitespace tag added to your "
         "messages, so that an OTR session can start automatically (default "
         "value, options otr.policy.* can override this value)"),
      "off", 0 },
    { "html_escape",
      N_("escape HTML special chars in encrypted messages sent (default "
         "value, options otr.policy.* can override this value)"),
      "off", 0 },
    { "html_filter",
      N_("remove HTML in decrypted messages received (default value, options "
         "otr.policy.* can override this value)"),
      "on", 0 },
};


/*
 * Callback for changes on options displayed in the OTR bar item: update the bar
 * item.
 */

void
otr_config_change_bar_cb (const void *pointer, void *data,
                          struct t_config_option *option)
{
    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) option;

    weechat_bar_item_update (OTR_BAR_ITEM);
}

/*
 * Free the compiled regex of option otr.look.no_send_tag_regex.
 */

void
otr_config_free_regex_no_send_tag (void)
{
    if (otr_config_regex_no_send_tag)
    {
        regfree (otr_config_regex_no_send_tag);
        free (otr_config_regex_no_send_tag);
        otr_config_regex_no_send_tag = NULL;
    }
}

/*
 * Compile the regex of option otr.look.no_send_tag_regex (POSIX extended,
 * case insensitive).
 *
 * If the option is empty or if the regex is invalid, there is no compiled
 * regex (the whitespace tag is not restricted).
 */

void
otr_config_compile_regex_no_send_tag (void)
{
    const char *ptr_regex;

    otr_config_free_regex_no_send_tag ();

    ptr_regex = weechat_config_string (otr_config_look_no_send_tag_regex);
    if (!ptr_regex || !ptr_regex[0])
        return;

    otr_config_regex_no_send_tag = malloc (
        sizeof (*otr_config_regex_no_send_tag));
    if (!otr_config_regex_no_send_tag)
        return;

    if (regcomp (otr_config_regex_no_send_tag, ptr_regex,
                 REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0)
    {
        free (otr_config_regex_no_send_tag);
        otr_config_regex_no_send_tag = NULL;
    }
}

/*
 * Callback for changes on option otr.look.no_send_tag_regex.
 */

void
otr_config_change_no_send_tag_regex_cb (const void *pointer, void *data,
                                        struct t_config_option *option)
{
    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) option;

    otr_config_compile_regex_no_send_tag ();
}

/*
 * Get the value inherited by a policy option (name without "otr.policy."), from
 * the options of upper levels: for example for "server.nick.peer.name", the
 * value of the first option found among "server.nick.name", "server.name" and
 * "otr.policy_default.name".
 *
 * Return the boolean value inherited, 0 if no option is found.
 */

int
otr_config_policy_inherited (struct t_config_file *config_file,
                             struct t_config_section *section,
                             const char *option_name)
{
    struct t_config_option *ptr_option;
    const char *ptr_name;
    char *prefix, *pos, *parent_name;

    ptr_name = strrchr (option_name, '.');
    if (!ptr_name)
        return 0;
    ptr_name++;

    prefix = weechat_strndup (option_name, ptr_name - 1 - option_name);
    if (!prefix)
        return 0;

    ptr_option = NULL;
    while (!ptr_option)
    {
        pos = strrchr (prefix, '.');
        if (pos)
        {
            pos[0] = '\0';
            weechat_asprintf (&parent_name, "%s.%s", prefix, ptr_name);
        }
        else
        {
            parent_name = NULL;
        }
        if (parent_name)
        {
            ptr_option = weechat_config_search_option (config_file, section,
                                                       parent_name);
            free (parent_name);
        }
        if (!pos)
            break;
    }

    if (!ptr_option)
    {
        ptr_option = weechat_config_search_option (
            config_file, otr_config_section_policy_default, ptr_name);
    }

    free (prefix);

    return (ptr_option) ? weechat_config_boolean (ptr_option) : 0;
}

/*
 * Check if a value is valid for a policy option: "toggle" or a boolean value
 * accepted by WeeChat (case sensitive, like in WeeChat core).
 *
 * Return 1 if the value is valid, 0 otherwise.
 */

int
otr_config_policy_value_valid (const char *value)
{
    const char *values[] = {
        "on", "yes", "y", "true", "t", "1",
        "off", "no", "n", "false", "f", "0",
        "toggle", NULL,
    };
    int i;

    if (!value)
        return 0;

    for (i = 0; values[i]; i++)
    {
        if (strcmp (values[i], value) == 0)
            return 1;
    }

    return 0;
}

/*
 * Check if the name of an option in section "policy" is valid: format
 * "server[.nick[.peer]].name", where name is a known policy, without empty
 * part.
 *
 * Return 1 if the name is valid, 0 otherwise.
 */

int
otr_config_policy_name_valid (const char *option_name)
{
    const char *ptr_name;
    int i;

    if (!option_name || !option_name[0] || (option_name[0] == '.')
        || strstr (option_name, ".."))
    {
        return 0;
    }

    ptr_name = strrchr (option_name, '.');
    if (!ptr_name)
        return 0;
    ptr_name++;

    for (i = 0; i < otr_config_policies_count; i++)
    {
        if (strcmp (otr_config_policies[i].name, ptr_name) == 0)
            return 1;
    }

    return 0;
}

/*
 * Callback to create an option in section "policy" (boolean, "off" if no value
 * is given); the option is not created if the name is not valid (see function
 * otr_config_policy_name_valid) or if the value is not a valid boolean
 * (the value is checked before the option is created: WeeChat runs the config
 * hooks when an option is created, see function
 * otr_modifier_policy_changed_cb).
 *
 * With value "toggle", the option is created with the opposite of the value
 * inherited from upper levels (see function otr_config_policy_inherited).
 */

int
otr_config_policy_create_option_cb (const void *pointer, void *data,
                                    struct t_config_file *config_file,
                                    struct t_config_section *section,
                                    const char *option_name, const char *value)
{
    struct t_config_option *ptr_option;
    const char *ptr_value;

    /* Make C compiler happy. */
    (void) pointer;
    (void) data;

    if (!otr_config_policy_name_valid (option_name))
        return WEECHAT_CONFIG_OPTION_SET_ERROR;
    ptr_value = (value) ? value : "off";
    if (!otr_config_policy_value_valid (ptr_value))
        return WEECHAT_CONFIG_OPTION_SET_ERROR;
    if (strcmp (ptr_value, "toggle") == 0)
    {
        ptr_value = (otr_config_policy_inherited (config_file, section,
                                                  option_name)) ?
            "off" : "on";
    }

    ptr_option = weechat_config_new_option (
        config_file, section,
        option_name, "boolean", "", NULL, 0, 0, ptr_value, ptr_value, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);

    return (ptr_option) ?
        WEECHAT_CONFIG_OPTION_SET_OK_CHANGED : WEECHAT_CONFIG_OPTION_SET_ERROR;
}

/*
 * Create an option in OTR configuration file, with the same default and current
 * value.
 *
 * Return pointer to new option, NULL if error.
 */

struct t_config_option *
otr_config_new_option (struct t_config_section *section, const char *name,
                       const char *type, const char *description,
                       const char *default_value,
                       void (*callback_change)(const void *pointer,
                                               void *data,
                                               struct t_config_option *option))
{
    return weechat_config_new_option (
        otr_config_file, section,
        name, type, description, NULL, 0, 0, default_value, default_value, 0,
        NULL, NULL, NULL,
        callback_change, NULL, NULL,
        NULL, NULL, NULL);
}

/*
 * Initialize OTR configuration file: create sections and options, then read
 * file otr.conf.
 *
 * Return 1 if OK, 0 if error.
 */

int
otr_config_init (void)
{
    int i, num_policy_defaults;

    otr_config_file = weechat_config_new (OTR_CONFIG_PRIO_NAME,
                                          NULL, NULL, NULL);
    if (!otr_config_file)
        return 0;

    /* Sections */
    otr_config_section_color = weechat_config_new_section (
        otr_config_file, "color",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    otr_config_section_look = weechat_config_new_section (
        otr_config_file, "look",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    otr_config_section_policy_default = weechat_config_new_section (
        otr_config_file, "policy_default",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    otr_config_section_policy = weechat_config_new_section (
        otr_config_file, "policy",
        1, 1,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        &otr_config_policy_create_option_cb, NULL, NULL,
        NULL, NULL, NULL);
    if (!otr_config_section_color || !otr_config_section_look
        || !otr_config_section_policy_default || !otr_config_section_policy)
    {
        return 0;
    }

    /* Colors */
    for (i = 0; i < OTR_CONFIG_NUM_COLOR_OPTIONS; i++)
    {
        otr_config_color[i] = otr_config_new_option (
            otr_config_section_color,
            otr_config_color_defs[i].name, "color",
            otr_config_color_defs[i].description,
            otr_config_color_defs[i].default_value,
            (otr_config_color_defs[i].update_bar) ?
            &otr_config_change_bar_cb : NULL);
    }

    /* Look */
    for (i = 0; i < OTR_CONFIG_NUM_LOOK_OPTIONS; i++)
    {
        otr_config_look[i] = otr_config_new_option (
            otr_config_section_look,
            otr_config_look_defs[i].name,
            /* The prefix contains color names. */
            (i == OTR_CONFIG_LOOK_PREFIX) ? "string|themable" : "string",
            otr_config_look_defs[i].description,
            otr_config_look_defs[i].default_value,
            (otr_config_look_defs[i].update_bar) ?
            &otr_config_change_bar_cb : NULL);
    }

    otr_config_look_debug = otr_config_new_option (
        otr_config_section_look, "debug", "boolean",
        N_("display debug messages in buffer \"OTR Debug\" (metadata only, "
           "never message contents)"),
        "off", NULL);
    otr_config_look_hints = otr_config_new_option (
        otr_config_section_look, "hints", "boolean",
        N_("give helpful hints on how to use this plugin and how to stay "
           "secure while using OTR (recommended)"),
        "on", NULL);
    otr_config_look_no_send_tag_regex = otr_config_new_option (
        otr_config_section_look, "no_send_tag_regex", "string",
        N_("do not add the OTR whitespace tag to messages sent to nicks "
           "matching this regex (POSIX extended, case insensitive)"),
        "^(alis|chanfix|global|.+serv|\\*.+)$",
        &otr_config_change_no_send_tag_regex_cb);

    /* Default policies */
    num_policy_defaults = (int)(sizeof (otr_config_policy_defaults)
                                / sizeof (otr_config_policy_defaults[0]));
    for (i = 0; i < num_policy_defaults; i++)
    {
        otr_config_new_option (otr_config_section_policy_default,
                               otr_config_policy_defaults[i].name, "boolean",
                               otr_config_policy_defaults[i].description,
                               otr_config_policy_defaults[i].default_value,
                               NULL);
    }

    weechat_config_read (otr_config_file);

    otr_config_compile_regex_no_send_tag ();

    return 1;
}
