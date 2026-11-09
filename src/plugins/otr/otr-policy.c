/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* OTR policies */

#include <regex.h>
#include <stdio.h>
#include <stdlib.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-config.h"
#include "otr-irc.h"
#include "otr-policy.h"


/*
 * Get value of an OTR policy for an account and a peer, searching the options
 * from the most specific to the most general: "server.nick.peer.name",
 * "server.nick.name", "server.name", "default.name".
 *
 * Return 1 if the policy is enabled, 0 if it is disabled or not found.
 */

int
otr_policy_for (const char *account, const char *peer, const char *name)
{
    char nick[128], option_name[640];
    const char *server;
    struct t_config_option *ptr_option;

    server = otr_irc_account_server (account);
    otr_irc_account_nick (account, nick, sizeof (nick));

    snprintf (option_name, sizeof (option_name),
              "%s.%s.%s.%s", server, nick, peer, name);
    ptr_option = weechat_config_search_option (otr_config_file,
                                               otr_config_section_policy,
                                               option_name);
    if (!ptr_option)
    {
        snprintf (option_name, sizeof (option_name),
                  "%s.%s.%s", server, nick, name);
        ptr_option = weechat_config_search_option (otr_config_file,
                                                   otr_config_section_policy,
                                                   option_name);
    }
    if (!ptr_option)
    {
        snprintf (option_name, sizeof (option_name), "%s.%s", server, name);
        ptr_option = weechat_config_search_option (otr_config_file,
                                                   otr_config_section_policy,
                                                   option_name);
    }
    if (!ptr_option)
    {
        snprintf (option_name, sizeof (option_name), "default.%s", name);
        ptr_option = weechat_config_search_option (otr_config_file,
                                                   otr_config_section_policy,
                                                   option_name);
    }

    return (ptr_option) ? weechat_config_boolean (ptr_option) : 0;
}

/*
 * Get value of an OTR policy for the account and the peer of a libotr context.
 *
 * Return 1 if the policy is enabled, 0 otherwise.
 */

int
otr_policy_ctx (ConnContext *context, const char *name)
{
    return otr_policy_for (context->accountname, context->username, name);
}

/*
 * Get the OTR query to send to a peer to start an OTR session, according to the
 * policies "allow_v2" and "allow_v3".
 *
 * Return the OTR query, NULL if OTR versions 2 and 3 are both disabled.
 */

const char *
otr_policy_query (const char *account, const char *peer)
{
    int allow_v2, allow_v3;

    allow_v2 = otr_policy_for (account, peer, "allow_v2");
    allow_v3 = otr_policy_for (account, peer, "allow_v3");

    if (allow_v2 && allow_v3)
        return "?OTRv23?";
    if (allow_v3)
        return "?OTRv3?";
    if (allow_v2)
        return "?OTRv2?";

    return NULL;
}

/*
 * Check if the whitespace tag must not be sent to a peer (nick matching the
 * regex of option otr.general.no_send_tag_regex).
 *
 * Return 1 if the tag must not be sent, 0 otherwise.
 */

int
otr_policy_no_send_tag (const char *peer)
{
    const char *ptr_regex;
    regex_t regex;
    int match;

    ptr_regex = weechat_config_string (otr_config_general_no_send_tag_regex);
    if (!ptr_regex || !ptr_regex[0])
        return 0;

    if (regcomp (&regex, ptr_regex, REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0)
        return 0;
    match = (regexec (&regex, peer, 0, NULL, 0) == 0) ? 1 : 0;
    regfree (&regex);

    return match;
}

/*
 * Build the list of default OTR policies, for display.
 *
 * Note: result must be freed after use.
 */

char *
otr_policy_format_default (void)
{
    char **result, option_name[128];
    struct t_config_option *ptr_option;
    int i;

    result = weechat_string_dyn_alloc (256);
    if (!result)
        return NULL;

    weechat_string_dyn_concat (result, _("Current default OTR policies:\n"),
                               -1);
    for (i = 0; i < OTR_CONFIG_NUM_POLICIES; i++)
    {
        snprintf (option_name, sizeof (option_name),
                  "default.%s", otr_config_policies[i].name);
        ptr_option = weechat_config_search_option (otr_config_file,
                                                   otr_config_section_policy,
                                                   option_name);
        weechat_string_dyn_concat (result, "  ", -1);
        weechat_string_dyn_concat (result, otr_config_policies[i].name, -1);
        weechat_string_dyn_concat (result, " (", -1);
        weechat_string_dyn_concat (result,
                                   _(otr_config_policies[i].description), -1);
        weechat_string_dyn_concat (result, ") : ", -1);
        weechat_string_dyn_concat (
            result,
            (ptr_option && weechat_config_boolean (ptr_option)) ? "on" : "off",
            -1);
        weechat_string_dyn_concat (result, "\n", -1);
    }
    weechat_string_dyn_concat (result,
                               _("Change default policies with: "
                                 "/otr policy default NAME on|off"),
                               -1);

    return weechat_string_dyn_free (result, 0);
}

/*
 * Build the list of OTR policies for a peer, for display.
 *
 * Note: result must be freed after use.
 */

char *
otr_policy_format_peer (const char *account, const char *peer)
{
    char **result, *header;
    int i;

    result = weechat_string_dyn_alloc (256);
    if (!result)
        return NULL;

    weechat_asprintf (&header, _("Current OTR policies for %s@%s:\n"),
                      peer, otr_irc_account_server (account));
    if (header)
    {
        weechat_string_dyn_concat (result, header, -1);
        free (header);
    }
    for (i = 0; i < OTR_CONFIG_NUM_POLICIES; i++)
    {
        weechat_string_dyn_concat (result, "  ", -1);
        weechat_string_dyn_concat (result, otr_config_policies[i].name, -1);
        weechat_string_dyn_concat (result, " (", -1);
        weechat_string_dyn_concat (result,
                                   _(otr_config_policies[i].description), -1);
        weechat_string_dyn_concat (result, ") : ", -1);
        weechat_string_dyn_concat (
            result,
            (otr_policy_for (account, peer, otr_config_policies[i].name)) ?
            "on" : "off",
            -1);
        weechat_string_dyn_concat (result, "\n", -1);
    }
    weechat_string_dyn_concat (
        result, _("Change policies with: /otr policy NAME on|off"), -1);

    return weechat_string_dyn_free (result, 0);
}
