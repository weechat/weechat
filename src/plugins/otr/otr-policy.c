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
#include "otr-util.h"


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
    char nick[128], opt[640];
    const char *server = otr_irc_account_server (account);
    struct t_config_option *o;

    otr_irc_account_nick (account, nick, sizeof (nick));

    snprintf (opt, sizeof (opt), "%s.%s.%s.%s", server, nick, peer, name);
    o = weechat_config_search_option (otr_config_file,
                                      otr_config_section_policy, opt);
    if (!o)
    {
        snprintf (opt, sizeof (opt), "%s.%s.%s", server, nick, name);
        o = weechat_config_search_option (otr_config_file,
                                          otr_config_section_policy, opt);
    }
    if (!o)
    {
        snprintf (opt, sizeof (opt), "%s.%s", server, name);
        o = weechat_config_search_option (otr_config_file,
                                          otr_config_section_policy, opt);
    }
    if (!o)
    {
        snprintf (opt, sizeof (opt), "default.%s", name);
        o = weechat_config_search_option (otr_config_file,
                                          otr_config_section_policy, opt);
    }
    return o ? weechat_config_boolean (o) : 0;
}

/*
 * Get value of an OTR policy for the account and the peer of a libotr context.
 *
 * Return 1 if the policy is enabled, 0 otherwise.
 */

int
otr_policy_ctx (ConnContext *c, const char *name)
{
    return otr_policy_for (c->accountname, c->username, name);
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
    const char *re = weechat_config_string (
        otr_config_general_no_send_tag_regex);
    regex_t r;
    int m;

    if (!re || !*re)
        return 0;
    if (regcomp (&r, re, REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0)
        return 0;
    m = (regexec (&r, peer, 0, NULL, 0) == 0);
    regfree (&r);
    return m;
}

/*
 * Build the list of default OTR policies, for display.
 *
 * Note: result must be freed after use.
 */

char *
otr_policy_format_default (void)
{
    struct t_otr_sbuf b;
    char opt[128];
    struct t_config_option *o;
    int i;

    otr_util_sb_init (&b);
    otr_util_sb_adds (&b, _("Current default OTR policies:\n"));
    for (i = 0; i < OTR_CONFIG_NUM_POLICIES; i++)
    {
        snprintf (opt, sizeof (opt), "default.%s", otr_config_policies[i].name);
        o = weechat_config_search_option (otr_config_file,
                                          otr_config_section_policy, opt);
        otr_util_sb_adds (&b, "  ");
        otr_util_sb_adds (&b, otr_config_policies[i].name);
        otr_util_sb_adds (&b, " (");
        otr_util_sb_adds (&b, _(otr_config_policies[i].desc));
        otr_util_sb_adds (&b, ") : ");
        otr_util_sb_adds (&b, (o && weechat_config_boolean (o)) ? "on" : "off");
        otr_util_sb_addc (&b, '\n');
    }
    otr_util_sb_adds (&b, _("Change default policies with: "
                            "/otr policy default NAME on|off"));
    return otr_util_sb_take (&b);
}

/*
 * Build the list of OTR policies for a peer, for display.
 *
 * Note: result must be freed after use.
 */

char *
otr_policy_format_peer (const char *account, const char *peer)
{
    struct t_otr_sbuf b;
    int i;

    otr_util_sb_init (&b);
    {
        char *hdr = otr_util_strf (_("Current OTR policies for %s@%s:\n"), peer,
                                   otr_irc_account_server (account));

        if (hdr)
            otr_util_sb_adds (&b, hdr);
        free (hdr);
    }
    for (i = 0; i < OTR_CONFIG_NUM_POLICIES; i++)
    {
        otr_util_sb_adds (&b, "  ");
        otr_util_sb_adds (&b, otr_config_policies[i].name);
        otr_util_sb_adds (&b, " (");
        otr_util_sb_adds (&b, _(otr_config_policies[i].desc));
        otr_util_sb_adds (&b, ") : ");
        otr_util_sb_adds (&b, otr_policy_for (account, peer,
                                              otr_config_policies[i].name)
                          ? "on" : "off");
        otr_util_sb_addc (&b, '\n');
    }
    otr_util_sb_adds (&b, _("Change policies with: /otr policy NAME on|off"));
    return otr_util_sb_take (&b);
}
