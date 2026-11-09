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


int
policy_for (const char *account, const char *peer, const char *name)
{
    char nick[128], opt[640];
    const char *server = account_server (account);
    struct t_config_option *o;

    account_nick (account, nick, sizeof (nick));

    snprintf (opt, sizeof (opt), "%s.%s.%s.%s", server, nick, peer, name);
    o = weechat_config_search_option (cfg_file, cfg_sec_policy, opt);
    if (!o)
    {
        snprintf (opt, sizeof (opt), "%s.%s.%s", server, nick, name);
        o = weechat_config_search_option (cfg_file, cfg_sec_policy, opt);
    }
    if (!o)
    {
        snprintf (opt, sizeof (opt), "%s.%s", server, name);
        o = weechat_config_search_option (cfg_file, cfg_sec_policy, opt);
    }
    if (!o)
    {
        snprintf (opt, sizeof (opt), "default.%s", name);
        o = weechat_config_search_option (cfg_file, cfg_sec_policy, opt);
    }
    return o ? weechat_config_boolean (o) : 0;
}

int
policy_ctx (ConnContext *c, const char *name)
{
    return policy_for (c->accountname, c->username, name);
}

int
no_send_tag (const char *peer)
{
    const char *re = weechat_config_string (opt_no_send_tag_regex);
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

char *
format_default_policies (void)
{
    struct sbuf b;
    char opt[128];
    struct t_config_option *o;
    int i;

    sb_init (&b);
    sb_adds (&b, _("Current default OTR policies:\n"));
    for (i = 0; i < N_POLICIES; i++)
    {
        snprintf (opt, sizeof (opt), "default.%s", policies[i].name);
        o = weechat_config_search_option (cfg_file, cfg_sec_policy, opt);
        sb_adds (&b, "  ");
        sb_adds (&b, policies[i].name);
        sb_adds (&b, " (");
        sb_adds (&b, _(policies[i].desc));
        sb_adds (&b, ") : ");
        sb_adds (&b, (o && weechat_config_boolean (o)) ? "on" : "off");
        sb_addc (&b, '\n');
    }
    sb_adds (&b, _("Change default policies with: "
                   "/otr policy default NAME on|off"));
    return sb_take (&b);
}

char *
format_peer_policies (const char *account, const char *peer)
{
    struct sbuf b;
    int i;

    sb_init (&b);
    {
        char *hdr = strf (_("Current OTR policies for %s@%s:\n"), peer,
                          account_server (account));

        if (hdr)
            sb_adds (&b, hdr);
        free (hdr);
    }
    for (i = 0; i < N_POLICIES; i++)
    {
        sb_adds (&b, "  ");
        sb_adds (&b, policies[i].name);
        sb_adds (&b, " (");
        sb_adds (&b, _(policies[i].desc));
        sb_adds (&b, ") : ");
        sb_adds (&b, policy_for (account, peer, policies[i].name)
                 ? "on" : "off");
        sb_addc (&b, '\n');
    }
    sb_adds (&b, _("Change policies with: /otr policy NAME on|off"));
    return sb_take (&b);
}
