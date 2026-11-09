/*
 * otr.c - Off-the-Record (OTR) messaging for WeeChat, written in C on top of
 *         libotr.
 *
 * C port of the feature set of the Python script "otr.py" (weechat-otr by
 * Matthew M. Boedicker, Nils Goers, Daniel Faucon, Felix Eckhofer), which
 * was built on the "potr" library. This plugin uses libotr instead, so it
 * speaks OTR protocol versions 2 and 3.
 *
 * Copyright (C) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * DISCLAIMER: no guarantee of any kind. Review before relying on it.
 */

#define _GNU_SOURCE

#include <ctype.h>
#include <dlfcn.h>
#include <regex.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include <gcrypt.h>
/* order matters: message.h needs the types from proto.h, tlv.h, context.h */
#include <libotr/version.h>
#include <libotr/proto.h>
#include <libotr/tlv.h>
#include <libotr/context.h>
#include <libotr/userstate.h>
#include <libotr/privkey.h>
#include <libotr/instag.h>
#include <libotr/message.h>
#include <libotr/version.h>

#if defined(__has_include)
#  if __has_include("../weechat-plugin.h")
#    include "../weechat-plugin.h"            /* in-tree: src/plugins/otr/ */
#  elif __has_include(<weechat/weechat-plugin.h>)
#    include <weechat/weechat-plugin.h>
#  else
#    include <weechat-plugin.h>
#  endif
#else
#  include <weechat/weechat-plugin.h>
#endif

WEECHAT_PLUGIN_NAME("otr");
WEECHAT_PLUGIN_DESCRIPTION(N_("Off-the-Record messaging for IRC (libotr)"));
WEECHAT_PLUGIN_AUTHOR("otr-c");
WEECHAT_PLUGIN_VERSION("0.1.0");
WEECHAT_PLUGIN_LICENSE("GPL3");

struct t_weechat_plugin *weechat_plugin = NULL;

#define OTR_PROTOCOL      "irc"
#define OTR_BAR_ITEM      "otr"
#define OTR_MAX_ASSEMBLY  (64 * 1024)
#define LOG_UNTOUCHED     (-2)   /* we have not changed the log level */
#define LOG_DEFAULT       (-1)   /* buffer used the default log level */
#define ACTION_PREFIX     "/me "
#define UNENCRYPTED_PFX   _("Unencrypted message received: ")

/* ------------------------------------------------------------------------ */
/* globals                                                                   */
/* ------------------------------------------------------------------------ */

static OtrlUserState otr_us = NULL;
static char *otr_dir = NULL;
static char *otr_keyfile = NULL;
static char *otr_fprfile = NULL;
static char *otr_instagfile = NULL;

static struct t_gui_bar_item *otr_bar_item = NULL;

/* >0 while we inject our own protocol messages through /quote, so that the
 * outgoing modifier lets them through untouched */
static int g_injecting = 0;
/* >0 while sending CTCP, which must not receive a whitespace tag */
static int g_no_tag = 0;
/* set while the plugin unloads: no UI work from libotr callbacks */
static int g_shutdown = 0;

/* ------------------------------------------------------------------------ */
/* small helpers                                                             */
/* ------------------------------------------------------------------------ */

static char *
strf (const char *fmt, ...)
{
    va_list ap, ap2;
    int n;
    char *s;

    va_start (ap, fmt);
    va_copy (ap2, ap);
    n = vsnprintf (NULL, 0, fmt, ap);
    va_end (ap);
    if (n < 0)
    {
        va_end (ap2);
        return NULL;
    }
    s = malloc ((size_t)n + 1);
    if (s)
        vsnprintf (s, (size_t)n + 1, fmt, ap2);
    va_end (ap2);
    return s;
}

static char *
dupstr (const char *s)
{
    return strdup (s ? s : "");
}

static char *
lc_dup (const char *s)
{
    char *r = dupstr (s), *p;

    if (!r)
        return NULL;
    for (p = r; *p; p++)
        *p = (char)tolower ((unsigned char)*p);
    return r;
}

struct sbuf
{
    char *s;
    size_t len, cap;
    int err;
};

static void
sb_init (struct sbuf *b)
{
    b->s = NULL;
    b->len = b->cap = 0;
    b->err = 0;
}

static void
sb_add (struct sbuf *b, const char *p, size_t n)
{
    size_t nc;
    char *t;

    if (b->err)
        return;
    if (b->len + n + 1 > b->cap)
    {
        nc = b->cap ? b->cap : 64;
        while (nc < b->len + n + 1)
            nc *= 2;
        t = realloc (b->s, nc);
        if (!t)
        {
            b->err = 1;
            return;
        }
        b->s = t;
        b->cap = nc;
    }
    memcpy (b->s + b->len, p, n);
    b->len += n;
    b->s[b->len] = '\0';
}

static void
sb_adds (struct sbuf *b, const char *s)
{
    sb_add (b, s, strlen (s));
}

static void
sb_addc (struct sbuf *b, char c)
{
    sb_add (b, &c, 1);
}

/* returns malloc'd string, or NULL on allocation failure */
static char *
sb_take (struct sbuf *b)
{
    char *r;

    if (b->err)
    {
        free (b->s);
        b->s = NULL;
        return NULL;
    }
    if (!b->s)
        return strdup ("");
    r = b->s;
    b->s = NULL;
    return r;
}

/* replace CR, LF, NUL-ish control characters that could break an IRC line */
static char *
irc_sanitize (const char *s)
{
    char *r = dupstr (s), *p;

    if (!r)
        return NULL;
    for (p = r; *p; p++)
        if (*p == '\r' || *p == '\n')
            *p = ' ';
    return r;
}

/* ------------------------------------------------------------------------ */
/* configuration                                                             */
/* ------------------------------------------------------------------------ */

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

enum
{
    LOOK_BAR_PREFIX, LOOK_BAR_ENCRYPTED, LOOK_BAR_UNENCRYPTED,
    LOOK_BAR_AUTHENTICATED, LOOK_BAR_UNAUTHENTICATED, LOOK_BAR_LOGGED,
    LOOK_BAR_NOTLOGGED, LOOK_BAR_SEPARATOR, LOOK_PREFIX,
    LOOK_COUNT
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

static const struct
{
    const char *name, *desc;
} policies[] = {
    { "allow_v2", N_("allow OTR protocol version 2") },
    { "allow_v3", N_("allow OTR protocol version 3") },
    { "require_encryption", N_("refuse to send unencrypted messages when OTR is "
      "enabled") },
    { "log", N_("enable logging of OTR conversations") },
    { "send_tag", N_("advertise your OTR capability using the whitespace tag") },
    { "html_escape", N_("escape HTML special characters in outbound messages") },
    { "html_filter", N_("filter HTML in incoming messages") },
};
#define N_POLICIES ((int)(sizeof (policies) / sizeof (policies[0])))

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

static struct t_config_file *cfg_file = NULL;
static struct t_config_section *cfg_sec_general = NULL;
static struct t_config_section *cfg_sec_color = NULL;
static struct t_config_section *cfg_sec_look = NULL;
static struct t_config_section *cfg_sec_policy = NULL;
static struct t_config_option *opt_debug = NULL;
static struct t_config_option *opt_hints = NULL;
static struct t_config_option *opt_no_send_tag_regex = NULL;
static struct t_config_option *opt_color[COL_COUNT];
static struct t_config_option *opt_look[LOOK_COUNT];

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

static int
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

/* ------------------------------------------------------------------------ */
/* output                                                                    */
/* ------------------------------------------------------------------------ */

static void
otr_debug (const char *fmt, ...)
{
    struct t_gui_buffer *b;
    va_list ap;
    char tmp[1024];

    if (!opt_debug || !weechat_config_boolean (opt_debug))
        return;
    b = weechat_buffer_search ("otr", "OTR Debug");
    if (!b)
    {
        b = weechat_buffer_new ("OTR Debug", NULL, NULL, NULL, NULL, NULL,
                                NULL);
        if (!b)
            return;
        weechat_buffer_set (b, "title", "OTR Debug");
        weechat_buffer_set (b, "localvar_set_no_log", "1");
    }
    va_start (ap, fmt);
    vsnprintf (tmp, sizeof (tmp), fmt, ap);
    va_end (ap);
    weechat_printf (b, "otr debug\t%s", tmp);
}

static void
otr_print_str (struct t_gui_buffer *buf, int col, const char *msg)
{
    char *prefix, *copy, *line, *nl;
    char colorbuf[64];
    const char *c;

    prefix = weechat_string_eval_expression (
        weechat_config_string (opt_look[LOOK_PREFIX]), NULL, NULL, NULL);
    c = weechat_color (weechat_config_color (opt_color[col]));
    snprintf (colorbuf, sizeof (colorbuf), "%s", c ? c : "");
    copy = dupstr (msg);
    if (!copy)
    {
        free (prefix);
        return;
    }
    line = copy;
    while (line)
    {
        nl = strchr (line, '\n');
        if (nl)
            *nl = '\0';
        weechat_printf (buf, "%s\t%s%s", prefix ? prefix : "otr", colorbuf,
                        line);
        line = nl ? nl + 1 : NULL;
    }
    free (copy);
    free (prefix);
}

static void
otr_error_core (const char *fmt, ...)
{
    char tmp[1024];
    va_list ap;

    va_start (ap, fmt);
    vsnprintf (tmp, sizeof (tmp), fmt, ap);
    va_end (ap);
    weechat_printf (NULL, "%sotr: %s", weechat_prefix ("error"), tmp);
}

/* ------------------------------------------------------------------------ */
/* IRC / buffer helpers                                                      */
/* ------------------------------------------------------------------------ */

static struct t_gui_buffer *
irc_buffer (const char *server, const char *nick)
{
    char *args = strf ("%s,%s", server, nick);
    const char *s;
    void *p = NULL;

    if (!args)
        return NULL;
    s = weechat_info_get ("irc_buffer", args);
    free (args);
    if (s && *s && sscanf (s, "%p", &p) == 1)
        return (struct t_gui_buffer *)p;
    return NULL;
}

static int
buf_is_private (struct t_gui_buffer *buf)
{
    const char *t;

    if (!buf)
        return 0;
    t = weechat_buffer_get_string (buf, "localvar_type");
    return t && strcmp (t, "private") == 0;
}

static int
target_is_channel (const char *server, const char *target)
{
    char *args;
    const char *r, *statusmsg;
    int res = 0;

    if (!target || !*target)
        return 0;
    args = strf ("%s,%s", server, target);
    if (args)
    {
        r = weechat_info_get ("irc_is_channel", args);
        res = (r && strcmp (r, "1") == 0);
        free (args);
    }
    if (!res)
    {
        /* status message prefix, e.g. "@#channel" */
        args = strf ("%s,STATUSMSG", server);
        if (args)
        {
            statusmsg = weechat_info_get ("irc_server_isupport_value", args);
            if (statusmsg && *statusmsg && strchr (statusmsg, target[0])
                && target[1])
            {
                free (args);
                args = strf ("%s,%s", server, target + 1);
                if (args)
                {
                    r = weechat_info_get ("irc_is_channel", args);
                    res = (r && strcmp (r, "1") == 0);
                }
            }
            free (args);
        }
    }
    return res;
}

/* "ownnick@server" (nick lowercased); NULL if not connected */
static char *
make_account (const char *server)
{
    const char *nick = weechat_info_get ("irc_nick", server);
    char *lc, *r;

    if (!nick || !*nick)
        return NULL;
    lc = lc_dup (nick);
    if (!lc)
        return NULL;
    r = strf ("%s@%s", lc, server);
    free (lc);
    return r;
}

static const char *
account_server (const char *account)
{
    const char *at = strchr (account, '@');

    return at ? at + 1 : "";
}

/* copy own nick part of account into dst */
static void
account_nick (const char *account, char *dst, size_t n)
{
    const char *at = strchr (account, '@');
    size_t l = at ? (size_t)(at - account) : strlen (account);

    if (l >= n)
        l = n - 1;
    memcpy (dst, account, l);
    dst[l] = '\0';
}

static void
peer_print (const char *server, const char *nick, int col, const char *msg)
{
    struct t_gui_buffer *buf = irc_buffer (server, nick);
    char *tmp = NULL;

    if (g_shutdown)
        return;

    if (nick && *nick && !buf_is_private (buf))
    {
        tmp = strf ("[%s] %s", nick, msg);
        if (tmp)
            msg = tmp;
    }
    otr_print_str (buf, col, msg);
    free (tmp);
}

static void
ctx_print (ConnContext *c, int col, const char *fmt, ...)
{
    va_list ap, ap2;
    int n;
    char *s;

    va_start (ap, fmt);
    va_copy (ap2, ap);
    n = vsnprintf (NULL, 0, fmt, ap);
    va_end (ap);
    if (n < 0)
    {
        va_end (ap2);
        return;
    }
    s = malloc ((size_t)n + 1);
    if (s)
    {
        vsnprintf (s, (size_t)n + 1, fmt, ap2);
        peer_print (account_server (c->accountname), c->username, col, s);
        free (s);
    }
    va_end (ap2);
}

static void
ctx_hint (ConnContext *c, const char *msg)
{
    if (weechat_config_boolean (opt_hints))
        ctx_print (c, LVL_HINT, "%s", msg);
}

static void
peer_hint (const char *server, const char *nick, const char *msg)
{
    if (weechat_config_boolean (opt_hints))
        peer_print (server, nick, LVL_HINT, msg);
}

/* ------------------------------------------------------------------------ */
/* policies                                                                  */
/* ------------------------------------------------------------------------ */

static int
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

static int
policy_ctx (ConnContext *c, const char *name)
{
    return policy_for (c->accountname, c->username, name);
}

static int
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

static char *
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

static char *
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

/* ------------------------------------------------------------------------ */
/* per-peer data (hung on the libotr master context)                         */
/* ------------------------------------------------------------------------ */

struct peer_data
{
    char *asm_buf;     /* reassembly of OTR messages split over IRC lines */
    size_t asm_len;
    int in_smp;
    int smp_question;
    int prev_log;      /* LOG_UNTOUCHED / LOG_DEFAULT / level */
};

static void
peer_data_free (void *p)
{
    struct peer_data *pd = p;

    if (!pd)
        return;
    free (pd->asm_buf);
    free (pd);
}

static struct peer_data *
peer_data_for (const char *account, const char *peer)
{
    ConnContext *m;
    struct peer_data *pd;

    m = otrl_context_find (otr_us, peer, account, OTR_PROTOCOL,
                           OTRL_INSTAG_MASTER, 1, NULL, NULL, NULL);
    if (!m)
        return NULL;
    if (!m->app_data)
    {
        pd = calloc (1, sizeof (*pd));
        if (!pd)
            return NULL;
        pd->prev_log = LOG_UNTOUCHED;
        m->app_data = pd;
        m->app_data_free = peer_data_free;
    }
    return m->app_data;
}

static struct peer_data *
peer_data_ctx (ConnContext *c)
{
    return peer_data_for (c->accountname, c->username);
}

static ConnContext *
find_ctx (const char *account, const char *peer)
{
    return otrl_context_find (otr_us, peer, account, OTR_PROTOCOL,
                              OTRL_INSTAG_BEST, 0, NULL, NULL, NULL);
}

static int
ctx_encrypted (ConnContext *c)
{
    return c && c->msgstate == OTRL_MSGSTATE_ENCRYPTED;
}

static int
ctx_verified (ConnContext *c)
{
    return ctx_encrypted (c) && c->active_fingerprint
        && otrl_context_is_fingerprint_trusted (c->active_fingerprint);
}

/* ------------------------------------------------------------------------ */
/* logging control of the buffer                                             */
/* ------------------------------------------------------------------------ */

static int
buf_is_logged (struct t_gui_buffer *buf)
{
    struct t_infolist *il;
    int res = 0;

    if (!buf)
        return 0;
    il = weechat_infolist_get ("logger_buffer", NULL, NULL);
    if (!il)
        return 0;
    while (weechat_infolist_next (il))
    {
        if (weechat_infolist_pointer (il, "buffer") == buf)
        {
            res = weechat_infolist_integer (il, "log_enabled") ? 1 : 0;
            break;
        }
    }
    weechat_infolist_free (il);
    return res;
}

static char *
logger_option_name (struct t_gui_buffer *buf)
{
    const char *plugin = weechat_buffer_get_string (buf, "plugin");
    const char *name = weechat_buffer_get_string (buf, "name");

    return strf ("logger.level.%s.%s", plugin ? plugin : "",
                 name ? name : "");
}

static int
buf_log_level (struct t_gui_buffer *buf)
{
    char *opt = logger_option_name (buf);
    struct t_infolist *il;
    int res = 0;

    if (!opt)
        return LOG_DEFAULT;
    if (!weechat_config_get (opt))
    {
        free (opt);
        return LOG_DEFAULT;
    }
    free (opt);
    il = weechat_infolist_get ("logger_buffer", NULL, NULL);
    if (il)
    {
        while (weechat_infolist_next (il))
        {
            if (weechat_infolist_pointer (il, "buffer") == buf)
            {
                res = weechat_infolist_integer (il, "log_level");
                break;
            }
        }
        weechat_infolist_free (il);
    }
    return res;
}

static void
log_disable (ConnContext *c, struct peer_data *pd)
{
    struct t_gui_buffer *buf;

    if (!pd || pd->prev_log != LOG_UNTOUCHED)
        return;
    buf = irc_buffer (account_server (c->accountname), c->username);
    if (!buf_is_private (buf))
        return;
    pd->prev_log = buf_log_level (buf);
    if (buf_is_logged (buf))
    {
        weechat_command (buf, "/mute logger disable");
        ctx_print (c, LVL_INFO,
                   _("Logs have been temporarily disabled for the session. "
                   "They will be restored upon finishing the OTR session."));
    }
}

static void
log_restore (ConnContext *c, struct peer_data *pd)
{
    struct t_gui_buffer *buf;
    char *cmd, *opt;

    if (!pd || pd->prev_log == LOG_UNTOUCHED)
        return;
    buf = irc_buffer (account_server (c->accountname), c->username);
    if (buf_is_private (buf))
    {
        if (pd->prev_log >= 0 && pd->prev_log < 10)
        {
            ctx_print (c, LVL_WARNING,
                       _("Restoring buffer logging value to: %d"), pd->prev_log);
            cmd = strf ("/mute logger set %d", pd->prev_log);
            if (cmd)
                weechat_command (buf, cmd);
            free (cmd);
        }
        else if (pd->prev_log == LOG_DEFAULT)
        {
            ctx_print (c, LVL_WARNING,
                       _("Restoring buffer logging value to default"));
            opt = logger_option_name (buf);
            cmd = opt ? strf ("/mute unset %s", opt) : NULL;
            if (cmd)
                weechat_command (buf, cmd);
            free (cmd);
            free (opt);
        }
    }
    pd->prev_log = LOG_UNTOUCHED;
}

/* ------------------------------------------------------------------------ */
/* message text conversion                                                   */
/* ------------------------------------------------------------------------ */

static int
is_ctcp (const char *t)
{
    size_t l = strlen (t);

    return l >= 2 && t[0] == '\x01' && t[l - 1] == '\x01';
}

static int
is_action (const char *t)
{
    return is_ctcp (t) && strncmp (t, "\x01" "ACTION ", 8) == 0;
}

/* "\x01ACTION text\x01" -> "/me text"; otherwise plain copy */
static char *
msg_plain_from_irc (const char *t)
{
    size_t l;

    if (!is_action (t))
        return dupstr (t);
    l = strlen (t);
    return strf (ACTION_PREFIX "%.*s", (int)(l - 9), t + 8);
}

static char *
html_escape (const char *in)
{
    struct sbuf b;

    sb_init (&b);
    for (; *in; in++)
    {
        if (*in == '&')
            sb_adds (&b, "&amp;");
        else if (*in == '<')
            sb_adds (&b, "&lt;");
        else if (*in == '>')
            sb_adds (&b, "&gt;");
        else
            sb_addc (&b, *in);
    }
    return sb_take (&b);
}

static void
utf8_append (struct sbuf *b, unsigned long cp)
{
    char t[4];

    if (cp == 0)
        return;
    if (cp < 0x80)
    {
        t[0] = (char)cp;
        sb_add (b, t, 1);
    }
    else if (cp < 0x800)
    {
        t[0] = (char)(0xC0 | (cp >> 6));
        t[1] = (char)(0x80 | (cp & 0x3F));
        sb_add (b, t, 2);
    }
    else if (cp < 0x10000)
    {
        t[0] = (char)(0xE0 | (cp >> 12));
        t[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        t[2] = (char)(0x80 | (cp & 0x3F));
        sb_add (b, t, 3);
    }
    else if (cp <= 0x10FFFF)
    {
        t[0] = (char)(0xF0 | (cp >> 18));
        t[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        t[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        t[3] = (char)(0x80 | (cp & 0x3F));
        sb_add (b, t, 4);
    }
}

/* Reduce HTML to IRC text: keep text, <br> -> newline, links -> [text](url),
 * decode a few entities, drop all other tags. */
static char *
html_filter (const char *in)
{
    struct sbuf b;
    char link[1024];
    size_t linkstart = 0;
    const char *p = in, *gt, *semi, *h;
    char name[16];
    int closing, k;

    link[0] = '\0';
    sb_init (&b);
    while (*p)
    {
        if (*p == '<' && (isalpha ((unsigned char)p[1]) || p[1] == '/'
                          || p[1] == '!')
            && (gt = strchr (p, '>')) != NULL)
        {
            const char *t = p + 1;
            closing = 0;
            if (*t == '/')
            {
                closing = 1;
                t++;
            }
            for (k = 0; k < (int)sizeof (name) - 1 && isalpha ((unsigned char)t[k]);
                 k++)
                name[k] = (char)tolower ((unsigned char)t[k]);
            name[k] = '\0';

            if (!closing && strcmp (name, "br") == 0)
                sb_addc (&b, '\n');
            else if (!closing && strcmp (name, "a") == 0)
            {
                /* look for href="..." inside the tag */
                char *content = strndup (p, (size_t)(gt - p));

                if (content)
                {
                    h = NULL;
                    for (const char *q = content; *q; q++)
                        if (strncasecmp (q, "href=", 5) == 0)
                        {
                            h = q + 5;
                            break;
                        }
                    if (h)
                    {
                        char quote = (*h == '"' || *h == '\'') ? *h : 0;
                        size_t n = 0;

                        if (quote)
                            h++;
                        while (h[n] && (quote ? h[n] != quote
                                              : !isspace ((unsigned char)h[n]))
                               && n < sizeof (link) - 1)
                            n++;
                        memcpy (link, h, n);
                        link[n] = '\0';
                        if (n > 0)
                        {
                            sb_addc (&b, '[');
                            linkstart = b.len;
                        }
                    }
                    free (content);
                }
            }
            else if (closing && strcmp (name, "a") == 0 && link[0])
            {
                size_t ll = strlen (link);

                if (b.s && b.len - linkstart == ll
                    && memcmp (b.s + linkstart, link, ll) == 0)
                    sb_addc (&b, ']');
                else
                {
                    sb_adds (&b, "](");
                    sb_adds (&b, link);
                    sb_addc (&b, ')');
                }
                link[0] = '\0';
            }
            p = gt + 1;
        }
        else if (*p == '&' && (semi = strchr (p, ';')) != NULL
                 && semi - p <= 10 && semi - p > 1)
        {
            size_t l = (size_t)(semi - p - 1);
            char ent[16];

            memcpy (ent, p + 1, l);
            ent[l] = '\0';
            if (ent[0] == '#')
            {
                char *end;
                unsigned long cp = (ent[1] == 'x' || ent[1] == 'X')
                    ? strtoul (ent + 2, &end, 16) : strtoul (ent + 1, &end, 10);

                if (*end == '\0' && l > 1 && cp > 0)
                    utf8_append (&b, cp);
                else
                    sb_add (&b, p, (size_t)(semi - p) + 1);
            }
            else if (strcmp (ent, "amp") == 0)
                sb_addc (&b, '&');
            else if (strcmp (ent, "lt") == 0)
                sb_addc (&b, '<');
            else if (strcmp (ent, "gt") == 0)
                sb_addc (&b, '>');
            else if (strcmp (ent, "quot") == 0)
                sb_addc (&b, '"');
            else if (strcmp (ent, "apos") == 0)
                sb_addc (&b, '\'');
            else if (strcmp (ent, "nbsp") == 0)
                sb_addc (&b, ' ');
            else
                sb_add (&b, p, (size_t)(semi - p) + 1);
            p = semi + 1;
        }
        else
        {
            sb_addc (&b, *p);
            p++;
        }
    }
    return sb_take (&b);
}

/* Add one decrypted line as an IRC protocol line to out. Removes CTCP
 * markers a peer could smuggle in; only a well-formed /me becomes ACTION. */
static void
add_decrypted_line (struct sbuf *out, const char *tags, const char *from,
                    const char *target, const char *line, size_t len)
{
    struct sbuf body;
    const char *p = line, *end = line + len;
    int action = 0;
    char *text;

    if (len >= 4 && strncmp (line, ACTION_PREFIX, 4) == 0)
    {
        action = 1;
        p += 4;
    }
    else if (len >= 9 && strncmp (line, "\x01" "ACTION ", 8) == 0
             && line[len - 1] == '\x01')
    {
        action = 1;
        p += 8;
        end--;
    }

    sb_init (&body);
    for (; p < end; p++)
        if (*p != '\x01' && *p != '\r' && *p != '\0')
            sb_addc (&body, *p);
    text = sb_take (&body);
    if (!text)
    {
        out->err = 1;
        return;
    }

    if (out->len)
        sb_adds (out, "\r\n");
    sb_adds (out, tags);
    sb_addc (out, ':');
    sb_adds (out, from);
    sb_adds (out, " PRIVMSG ");
    sb_adds (out, target);
    sb_adds (out, " :");
    if (action)
        sb_adds (out, "\x01" "ACTION ");
    sb_adds (out, text);
    if (action)
        sb_addc (out, '\x01');
    free (text);
}

/* msg may have several lines; each becomes its own PRIVMSG */
static char *
build_in_privmsgs (const char *tags, const char *from, const char *target,
                   const char *msg, const char *line_prefix, int decrypted)
{
    struct sbuf out;
    const char *p = msg, *nl;
    size_t l;

    sb_init (&out);
    while (*p)
    {
        nl = strchr (p, '\n');
        l = nl ? (size_t)(nl - p) : strlen (p);
        if (l && p[l - 1] == '\r')
            l--;
        if (decrypted)
            add_decrypted_line (&out, tags, from, target, p, l);
        else
        {
            struct sbuf ln;
            char *t;

            sb_init (&ln);
            sb_adds (&ln, line_prefix);
            sb_add (&ln, p, l);
            t = sb_take (&ln);
            if (t)
            {
                if (out.len)
                    sb_adds (&out, "\r\n");
                sb_adds (&out, tags);
                sb_addc (&out, ':');
                sb_adds (&out, from);
                sb_adds (&out, " PRIVMSG ");
                sb_adds (&out, target);
                sb_adds (&out, " :");
                for (char *q = t; *q; q++)
                    if (*q != '\r')
                        sb_addc (&out, *q);
                free (t);
            }
        }
        p = nl ? nl + 1 : p + strlen (p);
    }
    return sb_take (&out);
}

/* ------------------------------------------------------------------------ */
/* sending raw lines                                                         */
/* ------------------------------------------------------------------------ */

static void
send_raw (const char *server, const char *nick, const char *text)
{
    char *s = irc_sanitize (server), *n = irc_sanitize (nick),
         *t = irc_sanitize (text), *cmd = NULL;

    if (s && n && t)
        cmd = strf ("/quote -server %s PRIVMSG %s :%s", s, n, t);
    if (cmd)
    {
        otr_debug ("inject to %s on %s (len %d)", nick, server,
                   (int)strlen (text));
        g_injecting++;
        weechat_command (NULL, cmd);
        g_injecting--;
    }
    free (cmd);
    free (s);
    free (n);
    free (t);
}

/* ------------------------------------------------------------------------ */
/* libotr callbacks                                                          */
/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */
/* echo-message support: show the plaintext instead of the ciphertext        */
/* ------------------------------------------------------------------------ */

/* With the IRCv3 capability echo-message the server sends our own messages
 * back to us. That echo carries the ciphertext. We remember what we sent
 * (cipher line + the plaintext the user typed) and swap them back; echoes
 * of pure protocol traffic (query, AKE, fragments) are hidden. */

#define PENDING_MAX 32
#define PENDING_TTL 60

struct pending
{
    char *server, *peer, *cipher, *plain;
    time_t t;
};

static struct pending pending_q[PENDING_MAX];
static int pending_n = 0;

static void
pending_drop (int i)
{
    free (pending_q[i].server);
    free (pending_q[i].peer);
    free (pending_q[i].cipher);
    free (pending_q[i].plain);
    memmove (&pending_q[i], &pending_q[i + 1],
             (size_t)(pending_n - i - 1) * sizeof (struct pending));
    pending_n--;
    memset (&pending_q[pending_n], 0, sizeof (struct pending));
}

static void
pending_clear_all (void)
{
    while (pending_n > 0)
        pending_drop (0);
}

static void
pending_purge (void)
{
    time_t now = time (NULL);
    int i = 0;

    while (i < pending_n)
    {
        if (now - pending_q[i].t > PENDING_TTL)
            pending_drop (i);
        else
            i++;
    }
}

static void
pending_add (const char *server, const char *peer, const char *cipher,
             const char *plain)
{
    struct pending *p;

    pending_purge ();
    if (pending_n == PENDING_MAX)
        pending_drop (0);
    p = &pending_q[pending_n];
    p->server = dupstr (server);
    p->peer = lc_dup (peer);
    p->cipher = dupstr (cipher);
    p->plain = dupstr (plain);
    p->t = time (NULL);
    if (p->server && p->peer && p->cipher && p->plain)
        pending_n++;
    else
    {
        free (p->server);
        free (p->peer);
        free (p->cipher);
        free (p->plain);
        memset (p, 0, sizeof (*p));
    }
}

static size_t
trimmed_len (const char *s)
{
    size_t l = strlen (s);

    while (l && (s[l - 1] == ' ' || s[l - 1] == '\t'))
        l--;
    return l;
}

/* returns the plaintext (caller frees) or NULL if nothing matches */
static char *
pending_take (const char *server, const char *target, const char *cipher)
{
    int i;
    size_t cl = trimmed_len (cipher);
    char *res;

    pending_purge ();
    for (i = 0; i < pending_n; i++)
    {
        if (strcmp (pending_q[i].server, server) == 0
            && strcasecmp (pending_q[i].peer, target) == 0
            && trimmed_len (pending_q[i].cipher) == cl
            && memcmp (pending_q[i].cipher, cipher, cl) == 0)
        {
            res = pending_q[i].plain;
            pending_q[i].plain = NULL;
            pending_drop (i);
            return res;
        }
    }
    return NULL;
}

/* NULL = leave the echoed line alone; "" = hide it */
static char *
echo_replace (const char *server, const char *string, const char *host,
              const char *target, const char *text)
{
    char *plain, *tags = NULL, *res;
    const char *sp;

    if (strncmp (text, "?OTR", 4) != 0)
        return NULL;
    plain = pending_take (server, target, text);
    if (!plain)
        return strdup ("");
    if (string[0] == '@' && (sp = strchr (string, ' ')) != NULL)
        tags = strndup (string, (size_t)(sp - string) + 1);
    res = build_in_privmsgs (tags ? tags : "", host, target, plain, "", 1);
    free (tags);
    free (plain);
    return res ? res : strdup ("");
}

static OtrlPolicy
op_policy (void *opdata, ConnContext *ctx)
{
    OtrlPolicy p = 0;
    int v2 = policy_ctx (ctx, "allow_v2"), v3 = policy_ctx (ctx, "allow_v3");

    (void)opdata;
    if (!v2 && !v3)
        return 0;
    p = OTRL_POLICY_WHITESPACE_START_AKE | OTRL_POLICY_ERROR_START_AKE;
    if (v2)
        p |= OTRL_POLICY_ALLOW_V2;
    if (v3)
        p |= OTRL_POLICY_ALLOW_V3;
    if (policy_ctx (ctx, "require_encryption"))
        p |= OTRL_POLICY_REQUIRE_ENCRYPTION;
    if (!g_no_tag && policy_ctx (ctx, "send_tag")
        && !no_send_tag (ctx->username))
        p |= OTRL_POLICY_SEND_WHITESPACE_TAG;
    return p;
}

static void
op_create_privkey (void *opdata, const char *accountname,
                   const char *protocol)
{
    gcry_error_t err;

    (void)opdata;
    otr_print_str (NULL, LVL_INFO, _("Generating a new OTR key for this "
                   "account. This can take a few seconds and blocks WeeChat "
                   "meanwhile..."));
    err = otrl_privkey_generate (otr_us, otr_keyfile, accountname, protocol);
    if (err)
        otr_error_core (_("key generation failed: %s"), gcry_strerror (err));
    else
        otr_print_str (NULL, LVL_SUCCESS, _("OTR key generated."));
}

static void
op_create_instag (void *opdata, const char *accountname,
                  const char *protocol)
{
    (void)opdata;
    otrl_instag_generate (otr_us, otr_instagfile, accountname, protocol);
}

static int
op_is_logged_in (void *opdata, const char *accountname, const char *protocol,
                 const char *recipient)
{
    (void)opdata; (void)accountname; (void)protocol; (void)recipient;
    return -1; /* unknown */
}

static void
op_inject_message (void *opdata, const char *accountname,
                   const char *protocol, const char *recipient,
                   const char *message)
{
    (void)opdata; (void)protocol;
    send_raw (account_server (accountname), recipient, message);
}

static void
op_update_context_list (void *opdata)
{
    (void)opdata;
}

static void
op_write_fingerprints (void *opdata)
{
    (void)opdata;
    otrl_privkey_write_fingerprints (otr_us, otr_fprfile);
}

static int
op_max_message_size (void *opdata, ConnContext *ctx)
{
    int r = 415 - 10 - (int)strlen (ctx->username);

    (void)opdata;
    return r < 100 ? 100 : r;
}

static char *
verify_instructions (ConnContext *ctx)
{
    char own[OTRL_PRIVKEY_FPRINT_HUMAN_LEN], theirs[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];
    const char *server = account_server (ctx->accountname);

    strcpy (own, "?");
    strcpy (theirs, "?");
    otrl_privkey_fingerprint (otr_us, own, ctx->accountname, ctx->protocol);
    if (ctx->active_fingerprint)
        otrl_privkey_hash_to_human (theirs,
                                    ctx->active_fingerprint->fingerprint);
    return strf (
        _("You can verify that this contact is who they claim to be in\n"
        "one of the following ways:\n\n"
        "1) Verify each other's fingerprints using a secure channel:\n"
        "  Your fingerprint : %s\n"
        "  %s@%s's fingerprint : %s\n"
        "  then use the command: /otr trust %s %s\n\n"
        "2) SMP pre-shared secret that you both know:\n"
        "  /otr smp ask %s %s 'secret'\n\n"
        "3) SMP pre-shared secret that you both know with a question:\n"
        "  /otr smp ask %s %s <'question'> 'secret'\n\n"
        "Note: You can safely omit specifying the peer and server when\n"
        "      executing these commands from the appropriate conversation\n"
        "      buffer"),
        own, ctx->username, server, theirs, ctx->username, server,
        ctx->username, server, ctx->username, server);
}

static void
op_new_fingerprint (void *opdata, OtrlUserState us, const char *accountname,
                    const char *protocol, const char *username,
                    unsigned char fingerprint[20])
{
    char human[OTRL_PRIVKEY_FPRINT_HUMAN_LEN], *m;

    (void)opdata; (void)us; (void)protocol;
    otrl_privkey_hash_to_human (human, fingerprint);
    m = strf (_("New fingerprint: %s"), human);
    if (m)
        peer_print (account_server (accountname), username, LVL_WARNING, m);
    free (m);
}

static void
op_gone_secure (void *opdata, ConnContext *ctx)
{
    struct peer_data *pd = peer_data_ctx (ctx);
    int verified = ctx->active_fingerprint
        && otrl_context_is_fingerprint_trusted (ctx->active_fingerprint);
    char *instr;

    (void)opdata;

    /* disable logging before anything decrypted can be printed */
    if (!policy_ctx (ctx, "log"))
        log_disable (ctx, pd);
    else
    {
        struct t_gui_buffer *buf = irc_buffer (account_server (ctx->accountname),
                                               ctx->username);

        if (pd && pd->prev_log == LOG_UNTOUCHED && buf_is_private (buf))
            pd->prev_log = buf_log_level (buf);
        if (buf_is_logged (buf))
            ctx_hint (ctx,
                      _("You have enabled the recording to disk of OTR "
                      "conversations. By doing this you are potentially "
                      "putting yourself and your correspondent in danger. "
                      "Please consider disabling this policy with "
                      "\"/otr policy default log off\". To disable logging "
                      "for this OTR session, use \"/otr log stop\""));
    }

    if (verified)
        ctx_print (ctx, LVL_SUCCESS,
                   _("Authenticated secured OTR conversation started."));
    else
    {
        ctx_print (ctx, LVL_WARNING,
                   _("Unauthenticated secured OTR conversation started."));
        instr = verify_instructions (ctx);
        if (instr)
            ctx_hint (ctx, instr);
        free (instr);
    }
    weechat_bar_item_update (OTR_BAR_ITEM);
}

static void
op_gone_insecure (void *opdata, ConnContext *ctx)
{
    (void)opdata;
    if (g_shutdown)
    {
        log_restore (ctx, peer_data_ctx (ctx));
        return;
    }
    if (ctx->msgstate == OTRL_MSGSTATE_FINISHED)
        ctx_print (ctx, LVL_INFO,
                   _("%s has ended the private conversation. You should do "
                   "the same:\n/otr finish"), ctx->username);
    else
    {
        ctx_print (ctx, LVL_INFO, _("Private conversation ended."));
        log_restore (ctx, peer_data_ctx (ctx));
    }
    weechat_bar_item_update (OTR_BAR_ITEM);
}

static void
op_still_secure (void *opdata, ConnContext *ctx, int is_reply)
{
    (void)opdata; (void)is_reply;
    ctx_print (ctx, LVL_SUCCESS, _("Private conversation has been refreshed."));
    weechat_bar_item_update (OTR_BAR_ITEM);
}

static const char *
op_resent_msg_prefix (void *opdata, ConnContext *ctx)
{
    (void)opdata; (void)ctx;
    return strdup ("[resent] ");
}

static void
op_resent_msg_prefix_free (void *opdata, const char *prefix)
{
    (void)opdata;
    free ((void *)prefix);
}

static void
op_handle_smp_event (void *opdata, OtrlSMPEvent ev, ConnContext *ctx,
                     unsigned short progress, char *question)
{
    struct peer_data *pd = peer_data_ctx (ctx);

    (void)opdata;
    switch (ev)
    {
        case OTRL_SMPEVENT_ASK_FOR_SECRET:
            if (pd) { pd->in_smp = 1; pd->smp_question = 0; }
            ctx_print (ctx, LVL_INFO,
                       _("Peer has requested SMP verification.\n"
                       "Respond with: /otr smp respond <secret>"));
            break;
        case OTRL_SMPEVENT_ASK_FOR_ANSWER:
            if (pd) { pd->in_smp = 1; pd->smp_question = 1; }
            ctx_print (ctx, LVL_INFO,
                       _("Peer has requested SMP verification: %s\n"
                       "Respond with: /otr smp respond <answer>"),
                       question ? question : "");
            break;
        case OTRL_SMPEVENT_IN_PROGRESS:
            ctx_print (ctx, LVL_INFO, _("SMP progressing (%u%%)."),
                       (unsigned)progress);
            break;
        case OTRL_SMPEVENT_SUCCESS:
            /* make sure the fingerprint is remembered as SMP verified */
            if (ctx->active_fingerprint
                && !otrl_context_is_fingerprint_trusted (
                       ctx->active_fingerprint))
            {
                otrl_context_set_trust (ctx->active_fingerprint, "smp");
                otrl_privkey_write_fingerprints (otr_us, otr_fprfile);
            }
            ctx_print (ctx, LVL_SUCCESS, _("SMP verification succeeded."));
            if (pd && pd->smp_question)
                ctx_print (ctx, LVL_INFO,
                           _("You may want to authenticate your peer by "
                           "asking your own question:\n"
                           "/otr smp ask <'question'> 'secret'"));
            if (pd) { pd->in_smp = 0; pd->smp_question = 0; }
            break;
        case OTRL_SMPEVENT_FAILURE:
            ctx_print (ctx, LVL_ERROR, _("SMP verification failed."));
            if (pd) { pd->in_smp = 0; pd->smp_question = 0; }
            break;
        case OTRL_SMPEVENT_ABORT:
            ctx_print (ctx, LVL_WARNING, _("SMP aborted."));
            if (pd) { pd->in_smp = 0; pd->smp_question = 0; }
            break;
        case OTRL_SMPEVENT_CHEATED:
            ctx_print (ctx, LVL_ERROR,
                       _("SMP aborted: the peer behaved incorrectly."));
            if (pd) { pd->in_smp = 0; pd->smp_question = 0; }
            break;
        case OTRL_SMPEVENT_ERROR:
            ctx_print (ctx, LVL_ERROR, _("SMP error."));
            if (pd) { pd->in_smp = 0; pd->smp_question = 0; }
            break;
        default:
            break;
    }
    weechat_bar_item_update (OTR_BAR_ITEM);
}

static void
op_handle_msg_event (void *opdata, OtrlMessageEvent ev, ConnContext *ctx,
                     const char *message, gcry_error_t err)
{
    (void)opdata;
    if (!ctx)
        return;
    switch (ev)
    {
        case OTRL_MSGEVENT_ENCRYPTION_REQUIRED:
            ctx_print (ctx, LVL_ERROR,
                       _("Your message was NOT sent in the clear, because "
                       "policy requires an encrypted connection. It is held "
                       "back and will be sent once the OTR session is "
                       "established."));
            ctx_hint (ctx, _("Wait for the OTR connection or change the "
                      "policy to allow clear-text messages:\n"
                      "/otr policy require_encryption off"));
            break;
        case OTRL_MSGEVENT_ENCRYPTION_ERROR:
            ctx_print (ctx, LVL_ERROR,
                       _("An error occurred while encrypting your message. "
                       "It was NOT sent."));
            break;
        case OTRL_MSGEVENT_CONNECTION_ENDED:
            ctx_print (ctx, LVL_ERROR,
                       _("Your message was not sent. End your private "
                       "conversation:\n/otr finish"));
            break;
        case OTRL_MSGEVENT_SETUP_ERROR:
            ctx_print (ctx, LVL_ERROR,
                       _("Error setting up the private conversation: %s"),
                       err ? gcry_strerror (err) : "unknown error");
            break;
        case OTRL_MSGEVENT_MSG_REFLECTED:
            ctx_print (ctx, LVL_WARNING,
                       _("Received our own OTR message (reflected); ignored."));
            break;
        case OTRL_MSGEVENT_MSG_RESENT:
            ctx_print (ctx, LVL_INFO, _("The last message was resent."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_NOT_IN_PRIVATE:
            ctx_print (ctx, LVL_WARNING,
                       _("Received encrypted data but no private session "
                       "established."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_UNREADABLE:
            ctx_print (ctx, LVL_ERROR,
                       _("Received an unreadable encrypted message."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_MALFORMED:
            ctx_print (ctx, LVL_ERROR,
                       _("Received a malformed OTR message."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_GENERAL_ERR:
            ctx_print (ctx, LVL_ERROR, _("Received OTR error: %s"),
                       message ? message : "");
            break;
        case OTRL_MSGEVENT_RCVDMSG_UNRECOGNIZED:
            ctx_print (ctx, LVL_WARNING,
                       _("Received an unrecognized OTR message."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_FOR_OTHER_INSTANCE:
            ctx_print (ctx, LVL_INFO,
                       _("Received a message meant for another client "
                       "instance of yours; ignored."));
            break;
        case OTRL_MSGEVENT_RCVDMSG_UNENCRYPTED:
            /* reported with a message prefix in modifier_in_cb */
            break;
        default:
            otr_debug ("msg event %d", (int)ev);
            break;
    }
}

static const OtrlMessageAppOps otr_ops = {
    .policy = op_policy,
    .create_privkey = op_create_privkey,
    .is_logged_in = op_is_logged_in,
    .inject_message = op_inject_message,
    .update_context_list = op_update_context_list,
    .new_fingerprint = op_new_fingerprint,
    .write_fingerprints = op_write_fingerprints,
    .gone_secure = op_gone_secure,
    .gone_insecure = op_gone_insecure,
    .still_secure = op_still_secure,
    .max_message_size = op_max_message_size,
    .resent_msg_prefix = op_resent_msg_prefix,
    .resent_msg_prefix_free = op_resent_msg_prefix_free,
    .handle_smp_event = op_handle_smp_event,
    .handle_msg_event = op_handle_msg_event,
    .create_instag = op_create_instag,
};

/* ------------------------------------------------------------------------ */
/* modifiers (incoming / outgoing PRIVMSG)                                   */
/* ------------------------------------------------------------------------ */

static int
is_otr_query (const char *s)
{
    const char *p = s, *q;

    while ((p = strstr (p, "?OTR")) != NULL)
    {
        q = p + 4;
        if (*q == '?')
            return 1;
        if (*q == 'v')
        {
            q++;
            while (isalnum ((unsigned char)*q))
                q++;
            if (*q == '?')
                return 1;
        }
        p += 4;
    }
    return 0;
}

static int
has_otr_end (const char *s)
{
    size_t l = strlen (s);

    return l > 0 && (s[l - 1] == '.' || s[l - 1] == ',');
}

/* Add the line to the peer's assembly buffer; return 1 if a complete
 * message is now available (copied to *out, malloc'd). */
static int
assemble (struct peer_data *pd, const char *line, char **out)
{
    size_t l = strlen (line);
    char *nb;

    if (!pd->asm_buf && strncmp (line, "?OTR", 4) != 0)
    {
        *out = dupstr (line);
        return 1;
    }
    if (pd->asm_len + l + 1 > OTR_MAX_ASSEMBLY)
    {
        free (pd->asm_buf);
        pd->asm_buf = NULL;
        pd->asm_len = 0;
        *out = dupstr (line);
        return 1;
    }
    nb = realloc (pd->asm_buf, pd->asm_len + l + 1);
    if (!nb)
        return 0;
    pd->asm_buf = nb;
    memcpy (pd->asm_buf + pd->asm_len, line, l + 1);
    pd->asm_len += l;

    if (is_otr_query (pd->asm_buf) || has_otr_end (pd->asm_buf)
        || strncmp (pd->asm_buf, "?OTR", 4) != 0)
    {
        *out = pd->asm_buf;
        pd->asm_buf = NULL;
        pd->asm_len = 0;
        return 1;
    }
    return 0;
}

static char *
modifier_in_cb (const void *pointer, void *data, const char *modifier,
                const char *modifier_data, const char *string)
{
    struct t_hashtable *hin, *hout;
    const char *command, *nick, *host, *args, *sp, *text, *own;
    char *result = NULL, *target = NULL, *account = NULL, *peer = NULL;
    char *assembled = NULL, *newmsg = NULL, *tags = NULL, *deliver = NULL;
    const char *server = modifier_data;
    struct peer_data *pd;
    OtrlTLV *tlvs = NULL;
    ConnContext *ctx = NULL;
    int ignore, was_otr, encrypted;

    (void)pointer; (void)data; (void)modifier;

    if (!otr_us)
        return NULL;
    hin = weechat_hashtable_new (8, WEECHAT_HASHTABLE_STRING,
                                 WEECHAT_HASHTABLE_STRING, NULL, NULL);
    if (!hin)
        return NULL;
    weechat_hashtable_set (hin, "message", string);
    hout = weechat_info_get_hashtable ("irc_message_parse", hin);
    weechat_hashtable_free (hin);
    if (!hout)
        return NULL;

    command = weechat_hashtable_get (hout, "command");
    nick = weechat_hashtable_get (hout, "nick");
    host = weechat_hashtable_get (hout, "host");
    args = weechat_hashtable_get (hout, "arguments");
    if (!command || strcasecmp (command, "PRIVMSG") != 0 || !nick || !*nick
        || !host || !*host || !args)
        goto done;

    sp = strchr (args, ' ');
    if (!sp)
        goto done;
    target = strndup (args, (size_t)(sp - args));
    text = sp + 1;
    if (*text == ':')
        text++;
    if (!target || target_is_channel (server, target))
        goto done;

    own = weechat_info_get ("irc_nick", server);
    if (own && strcasecmp (own, nick) == 0)
    {
        /* echo-message: the server returns our own outgoing message */
        result = echo_replace (server, string, host, target, text);
        goto done;
    }

    /* CTCP: only a plaintext ACTION inside an encrypted session is flagged */
    account = make_account (server);
    if (!account)
        goto done;
    peer = lc_dup (nick);
    if (!peer)
        goto done;

    if (is_ctcp (text))
    {
        if (is_action (text) && ctx_encrypted (find_ctx (account, peer)))
        {
            char *plain = msg_plain_from_irc (text);

            if (plain)
            {
                if (string[0] == '@' && (sp = strchr (string, ' ')) != NULL)
                    tags = strndup (string, (size_t)(sp - string) + 1);
                result = build_in_privmsgs (tags ? tags : "", host, target,
                                            plain, UNENCRYPTED_PFX, 0);
                free (plain);
            }
        }
        goto done;
    }

    pd = peer_data_for (account, peer);
    if (!pd || !assemble (pd, text, &assembled))
    {
        if (pd)
            result = strdup (""); /* waiting for more parts */
        goto done;
    }

    was_otr = (strncmp (assembled, "?OTR", 4) == 0);
    ignore = otrl_message_receiving (otr_us, &otr_ops, NULL, account,
                                     OTR_PROTOCOL, peer, assembled, &newmsg,
                                     &tlvs, &ctx, NULL, NULL);
    if (tlvs)
        otrl_tlv_free (tlvs);

    if (string[0] == '@' && (sp = strchr (string, ' ')) != NULL)
        tags = strndup (string, (size_t)(sp - string) + 1);

    if (ignore)
    {
        result = strdup ("");
    }
    else
    {
        encrypted = ctx_encrypted (ctx ? ctx : find_ctx (account, peer));

        if (newmsg && was_otr && encrypted)
        {
            /* decrypted message */
            if (policy_for (account, peer, "html_filter"))
                deliver = html_filter (newmsg);
            else
                deliver = dupstr (newmsg);
            if (deliver)
                result = build_in_privmsgs (tags ? tags : "", host, target,
                                            deliver, "", 1);
            else
                result = strdup ("");
        }
        else if (encrypted && !was_otr)
        {
            /* plaintext although the session is encrypted */
            result = build_in_privmsgs (tags ? tags : "", host, target,
                                        newmsg ? newmsg : assembled,
                                        UNENCRYPTED_PFX, 0);
        }
        else if (newmsg)
        {
            /* e.g. whitespace tag stripped from plaintext */
            result = build_in_privmsgs (tags ? tags : "", host, target,
                                        newmsg, "", 0);
        }
        else if (strcmp (assembled, text) != 0)
        {
            result = build_in_privmsgs (tags ? tags : "", host, target,
                                        assembled, "", 0);
        }
        /* otherwise: unchanged (result == NULL) */
    }
    if (newmsg)
        otrl_message_free (newmsg);
    weechat_bar_item_update (OTR_BAR_ITEM);

done:
    free (deliver);
    free (tags);
    free (assembled);
    free (target);
    free (account);
    free (peer);
    weechat_hashtable_free (hout);
    return result;
}

/* "PRIVMSG target :text" */
static int
parse_out_privmsg (const char *s, char **target, const char **text)
{
    const char *sp;

    while (*s == ' ')
        s++;
    if (strncasecmp (s, "PRIVMSG ", 8) != 0)
        return 0;
    s += 8;
    while (*s == ' ')
        s++;
    sp = strchr (s, ' ');
    if (!sp)
        return 0;
    *target = strndup (s, (size_t)(sp - s));
    if (!*target)
        return 0;
    sp++;
    if (*sp == ':')
        sp++;
    *text = sp;
    return 1;
}

/* 1 = all comma separated targets are channels */
static int
all_targets_channels (const char *server, const char *list)
{
    char *copy = dupstr (list), *save = NULL, *t;
    int ok = 1;

    if (!copy)
        return 0;
    for (t = strtok_r (copy, ",", &save); t; t = strtok_r (NULL, ",", &save))
        if (!target_is_channel (server, t))
        {
            ok = 0;
            break;
        }
    free (copy);
    return ok;
}

/*
 * Fails closed: any problem returns an empty string, which makes WeeChat
 * drop the message instead of sending it unencrypted.
 */
static char *
modifier_out_cb (const void *pointer, void *data, const char *modifier,
                 const char *modifier_data, const char *string)
{
    char *target = NULL, *account = NULL, *peer = NULL, *plain = NULL;
    char *newmsg = NULL, *result = NULL, *clean = NULL, *shown = NULL;
    const char *text, *server = modifier_data;
    ConnContext *ctx;
    gcry_error_t err;
    int ctcp;

    (void)pointer; (void)data; (void)modifier;

    if (g_injecting)
        return NULL;
    if (!otr_us)
        return strdup ("");

    if (!parse_out_privmsg (string, &target, &text))
    {
        otr_error_core (_("could not parse an outgoing message; it was not "
                        "sent"));
        return strdup ("");
    }

    if (strchr (target, ','))
    {
        if (all_targets_channels (server, target))
        {
            free (target);
            return NULL;
        }
        otr_error_core (_("messages to several targets including a nick are "
                        "not supported with OTR; not sent"));
        free (target);
        return strdup ("");
    }
    if (target_is_channel (server, target))
    {
        free (target);
        return NULL;
    }

    account = make_account (server);
    peer = lc_dup (target);
    if (!account || !peer)
    {
        otr_error_core (_("not connected to server %s; message not sent"),
                        server);
        result = strdup ("");
        goto done;
    }

    ctx = find_ctx (account, peer);
    if (ctx_encrypted (ctx))
    {
        char *conv = msg_plain_from_irc (text);

        shown = dupstr (conv);
        if (conv && policy_for (account, peer, "html_escape"))
        {
            plain = html_escape (conv);
            free (conv);
        }
        else
            plain = conv;
    }
    else
        plain = dupstr (text);
    if (!plain)
    {
        result = strdup ("");
        goto done;
    }

    ctcp = is_ctcp (text);
    if (ctcp)
        g_no_tag++;
    err = otrl_message_sending (otr_us, &otr_ops, NULL, account,
                                OTR_PROTOCOL, peer, OTRL_INSTAG_BEST, plain,
                                NULL, &newmsg, OTRL_FRAGMENT_SEND_ALL_BUT_LAST,
                                NULL, NULL, NULL);
    if (ctcp)
        g_no_tag--;

    if (err)
    {
        peer_print (server, peer, LVL_ERROR, _("Failed to send message; it was "
                    "NOT sent."));
        otr_debug ("otrl_message_sending: %s", gcry_strerror (err));
        result = strdup ("");
        goto done;
    }

    if (newmsg)
    {
        clean = irc_sanitize (newmsg);
        if (!clean || !*clean)
            result = strdup ("");
        else
        {
            size_t l = strlen (clean);

            if (shown && strncmp (clean, "?OTR", 4) == 0)
                pending_add (server, peer, clean, shown);

            /* WeeChat strips trailing spaces; keep whitespace tags intact */
            result = strf ("PRIVMSG %s :%s%s", target, clean,
                           clean[l - 1] == ' ' ? "\t" : "");
        }
        otrl_message_free (newmsg);
    }
    /* else: not an OTR message (plaintext allowed): result stays NULL */

    weechat_bar_item_update (OTR_BAR_ITEM);

done:
    free (shown);
    free (clean);
    free (plain);
    free (target);
    free (account);
    free (peer);
    return result;
}

/* ------------------------------------------------------------------------ */
/* status bar                                                                */
/* ------------------------------------------------------------------------ */

static void
bar_part (struct sbuf *b, const char *sep, int *first, int color_idx,
          int look_idx)
{
    const char *str = weechat_config_string (opt_look[look_idx]);
    char col[64], def[64];

    if (!str || !*str)
        return;
    snprintf (col, sizeof (col), "%s",
              weechat_color (weechat_config_color (opt_color[color_idx])));
    snprintf (def, sizeof (def), "%s",
              weechat_color (weechat_config_color (
                  opt_color[COL_STATUS_DEFAULT])));
    if (!*first)
        sb_adds (b, sep);
    *first = 0;
    sb_adds (b, col);
    sb_adds (b, str);
    sb_adds (b, def);
}

static char *
bar_item_cb (const void *pointer, void *data, struct t_gui_bar_item *item,
             struct t_gui_window *window, struct t_gui_buffer *buffer,
             struct t_hashtable *extra_info)
{
    struct t_gui_buffer *buf = buffer ? buffer : weechat_current_buffer ();
    const char *own, *server, *peer;
    char *account, *peerlc, *res;
    ConnContext *ctx;
    struct sbuf parts, out;
    int first = 1, enc, ver, logged;
    char col[64];

    (void)pointer; (void)data; (void)item; (void)window; (void)extra_info;

    if (!otr_us || !buf_is_private (buf))
        return strdup ("");

    own = weechat_buffer_get_string (buf, "localvar_nick");
    server = weechat_buffer_get_string (buf, "localvar_server");
    peer = weechat_buffer_get_string (buf, "localvar_channel");
    if (!own || !server || !peer)
        return strdup ("");
    {
        char *l = lc_dup (own);

        account = l ? strf ("%s@%s", l, server) : NULL;
        free (l);
    }
    peerlc = lc_dup (peer);
    if (!account || !peerlc)
    {
        free (account);
        free (peerlc);
        return strdup ("");
    }
    ctx = find_ctx (account, peerlc);
    enc = ctx_encrypted (ctx);
    ver = ctx_verified (ctx);
    logged = buf_is_logged (buf);

    sb_init (&parts);
    if (enc)
    {
        bar_part (&parts, weechat_config_string (opt_look[LOOK_BAR_SEPARATOR]),
                  &first, COL_STATUS_ENCRYPTED, LOOK_BAR_ENCRYPTED);
        if (ver)
            bar_part (&parts,
                      weechat_config_string (opt_look[LOOK_BAR_SEPARATOR]),
                      &first, COL_STATUS_AUTHENTICATED, LOOK_BAR_AUTHENTICATED);
        else
            bar_part (&parts,
                      weechat_config_string (opt_look[LOOK_BAR_SEPARATOR]),
                      &first, COL_STATUS_UNAUTHENTICATED,
                      LOOK_BAR_UNAUTHENTICATED);
        if (logged)
            bar_part (&parts,
                      weechat_config_string (opt_look[LOOK_BAR_SEPARATOR]),
                      &first, COL_STATUS_LOGGED, LOOK_BAR_LOGGED);
        else
            bar_part (&parts,
                      weechat_config_string (opt_look[LOOK_BAR_SEPARATOR]),
                      &first, COL_STATUS_NOTLOGGED, LOOK_BAR_NOTLOGGED);
    }
    else
        bar_part (&parts, weechat_config_string (opt_look[LOOK_BAR_SEPARATOR]),
                  &first, COL_STATUS_UNENCRYPTED, LOOK_BAR_UNENCRYPTED);

    sb_init (&out);
    if (parts.len)
    {
        snprintf (col, sizeof (col), "%s",
                  weechat_color (weechat_config_color (
                      opt_color[COL_STATUS_DEFAULT])));
        sb_adds (&out, col);
        sb_adds (&out, weechat_config_string (opt_look[LOOK_BAR_PREFIX]));
        sb_adds (&out, parts.s);
    }
    free (parts.s);
    res = sb_take (&out);

    weechat_buffer_set (buf, "localvar_set_otr_encrypted",
                        enc ? "true" : "false");
    weechat_buffer_set (buf, "localvar_set_otr_authenticated",
                        ver ? "true" : "false");
    weechat_buffer_set (buf, "localvar_set_otr_logged",
                        logged ? "true" : "false");

    free (account);
    free (peerlc);
    return res ? res : strdup ("");
}

/* ------------------------------------------------------------------------ */
/* commands                                                                  */
/* ------------------------------------------------------------------------ */

/* shlex-like splitting; returns count or -1 on error. Caller frees tokens. */
static int
split_args (const char *s, char **out, int max)
{
    const char *p = s;
    int n = 0, i;

    while (*p)
    {
        char *tok, *w, q = 0;

        while (isspace ((unsigned char)*p))
            p++;
        if (!*p)
            break;
        if (n >= max)
            goto fail;
        tok = malloc (strlen (p) + 1);
        if (!tok)
            goto fail;
        w = tok;
        while (*p && (q || !isspace ((unsigned char)*p)))
        {
            if (q == '\'')
            {
                if (*p == '\'')
                    q = 0;
                else
                    *w++ = *p;
                p++;
            }
            else if (q == '"')
            {
                if (*p == '"')
                {
                    q = 0;
                    p++;
                }
                else if (*p == '\\' && (p[1] == '"' || p[1] == '\\'))
                {
                    *w++ = p[1];
                    p += 2;
                }
                else
                    *w++ = *p++;
            }
            else if (*p == '\'' || *p == '"')
            {
                q = *p;
                p++;
            }
            else if (*p == '\\' && p[1])
            {
                *w++ = p[1];
                p += 2;
            }
            else
                *w++ = *p++;
        }
        if (q)
        {
            free (tok);
            goto fail;
        }
        *w = '\0';
        out[n++] = tok;
    }
    return n;
fail:
    for (i = 0; i < n; i++)
        free (out[i]);
    return -1;
}

struct peer_ref
{
    char nick[128];
    char server[128];
};

/* [NICK SERVER] arguments or, with none, the current private buffer */
static int
peer_args (struct t_gui_buffer *buf, char **args, int nargs,
           struct peer_ref *p)
{
    const char *n, *s;

    if (nargs == 2)
    {
        snprintf (p->nick, sizeof (p->nick), "%s", args[0]);
        snprintf (p->server, sizeof (p->server), "%s", args[1]);
        return 1;
    }
    if (nargs == 0 && buf_is_private (buf))
    {
        n = weechat_buffer_get_string (buf, "localvar_channel");
        s = weechat_buffer_get_string (buf, "localvar_server");
        if (n && s)
        {
            snprintf (p->nick, sizeof (p->nick), "%s", n);
            snprintf (p->server, sizeof (p->server), "%s", s);
            return 1;
        }
    }
    return 0;
}

/* account + lowercased peer for a peer_ref; prints an error on failure */
static int
resolve (const struct peer_ref *p, char **account, char **peer)
{
    *account = make_account (p->server);
    *peer = lc_dup (p->nick);
    if (!*account || !*peer)
    {
        otr_error_core (_("not connected to server \"%s\""), p->server);
        free (*account);
        free (*peer);
        *account = *peer = NULL;
        return 0;
    }
    return 1;
}

struct table
{
    char *(*rows)[4];
    int n, cap;
    size_t w[4];
};

static void
table_add (struct table *t, const char *a, const char *b, const char *c,
           const char *d)
{
    const char *v[4];
    int i;
    void *nr;

    v[0] = a; v[1] = b; v[2] = c; v[3] = d;
    if (t->n == t->cap)
    {
        t->cap = t->cap ? t->cap * 2 : 8;
        nr = realloc (t->rows, (size_t)t->cap * sizeof (t->rows[0]));
        if (!nr)
            return;
        t->rows = nr;
    }
    for (i = 0; i < 4; i++)
    {
        t->rows[t->n][i] = dupstr (v[i]);
        if (t->rows[t->n][i] && strlen (t->rows[t->n][i]) > t->w[i])
            t->w[i] = strlen (t->rows[t->n][i]);
    }
    t->n++;
}

static void
table_print_free (struct table *t, int cols)
{
    int r, c;
    struct sbuf b;
    char *line;

    for (r = 0; r < t->n; r++)
    {
        sb_init (&b);
        for (c = 0; c < cols; c++)
        {
            size_t l = strlen (t->rows[r][c]);

            if (c)
                sb_adds (&b, " |");
            sb_adds (&b, t->rows[r][c]);
            while (l++ < t->w[c])
                sb_addc (&b, ' ');
        }
        line = sb_take (&b);
        if (line)
            otr_print_str (NULL, LVL_INFO, line);
        free (line);
        for (c = 0; c < 4; c++)
            free (t->rows[r][c]);
    }
    free (t->rows);
}

static void
show_account_fingerprints (void)
{
    struct table t = { NULL, 0, 0, { 0, 0, 0, 0 } };
    OtrlPrivKey *k;
    char human[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];

    for (k = otr_us->privkey_root; k; k = k->next)
    {
        if (!otrl_privkey_fingerprint (otr_us, human, k->accountname,
                                       k->protocol))
            continue;
        table_add (&t, k->accountname, human, "", "");
    }
    table_print_free (&t, 2);
}

static void
show_peer_fingerprints (const char *grep)
{
    struct table t = { NULL, 0, 0, { 0, 0, 0, 0 } };
    ConnContext *c;
    Fingerprint *f;
    char human[OTRL_PRIVKEY_FPRINT_HUMAN_LEN], peerfull[256];
    const char *trust;

    for (c = otr_us->context_root; c; c = c->next)
    {
        if (c->their_instance != OTRL_INSTAG_MASTER)
            continue;
        snprintf (peerfull, sizeof (peerfull), "%s", c->username);
        if (grep && !strstr (peerfull, grep))
            continue;
        for (f = c->fingerprint_root.next; f; f = f->next)
        {
            otrl_privkey_hash_to_human (human, f->fingerprint);
            if (!f->trust || !*f->trust)
                trust = _("unverified");
            else if (strcmp (f->trust, "smp") == 0)
                trust = _("SMP verified");
            else if (strcmp (f->trust, "verified") == 0)
                trust = _("verified");
            else
                trust = f->trust;
            table_add (&t, peerfull, c->accountname, human, trust);
        }
    }
    table_print_free (&t, 4);
}

static int
cmd_start (struct t_gui_buffer *buf, const struct peer_ref *p)
{
    char *account, *peer;
    int v2, v3;
    const char *q;

    (void)buf;
    if (!resolve (p, &account, &peer))
        return WEECHAT_RC_ERROR;
    v2 = policy_for (account, peer, "allow_v2");
    v3 = policy_for (account, peer, "allow_v3");
    if (!v2 && !v3)
    {
        peer_print (p->server, peer, LVL_ERROR,
                    _("OTR is disabled by policy (allow_v2 and allow_v3 are "
                    "both off)."));
        free (account);
        free (peer);
        return WEECHAT_RC_OK;
    }
    q = (v2 && v3) ? "?OTRv23?" : (v3 ? "?OTRv3?" : "?OTRv2?");
    peer_hint (p->server, peer,
               _("Sending OTR query... Please await confirmation of the OTR "
               "session being started before sending a message."));
    if (!policy_for (account, peer, "send_tag"))
    {
        char *m = strf (_("To try OTR on all conversations with %s@%s: /otr "
                        "policy send_tag on"), peer, p->server);

        if (m)
            peer_hint (p->server, peer, m);
        free (m);
    }
    send_raw (p->server, peer, q);
    free (account);
    free (peer);
    return WEECHAT_RC_OK;
}

static int
cmd_finish (const struct peer_ref *p)
{
    char *account, *peer;
    ConnContext *ctx;

    if (!resolve (p, &account, &peer))
        return WEECHAT_RC_ERROR;
    ctx = find_ctx (account, peer);
    if (ctx && ctx->msgstate != OTRL_MSGSTATE_PLAINTEXT)
        otrl_message_disconnect_all_instances (otr_us, &otr_ops, NULL, account,
                                               OTR_PROTOCOL, peer);
    else
        peer_print (p->server, peer, LVL_INFO,
                    _("There is no private conversation to finish."));
    weechat_bar_item_update (OTR_BAR_ITEM);
    free (account);
    free (peer);
    return WEECHAT_RC_OK;
}

static int
cmd_status (const struct peer_ref *p)
{
    char *account, *peer;
    ConnContext *ctx;
    char own[OTRL_PRIVKEY_FPRINT_HUMAN_LEN], theirs[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];
    char *m;

    if (!resolve (p, &account, &peer))
        return WEECHAT_RC_ERROR;
    ctx = find_ctx (account, peer);
    if (ctx_encrypted (ctx))
    {
        peer_print (p->server, peer, LVL_SUCCESS,
                    _("This conversation is encrypted."));
        strcpy (own, "?");
        strcpy (theirs, "?");
        otrl_privkey_fingerprint (otr_us, own, account, OTR_PROTOCOL);
        if (ctx->active_fingerprint)
            otrl_privkey_hash_to_human (theirs,
                                        ctx->active_fingerprint->fingerprint);
        m = strf (_("Your fingerprint is: %s"), own);
        if (m)
            peer_print (p->server, peer, LVL_INFO, m);
        free (m);
        m = strf (_("Your peer's fingerprint is: %s"), theirs);
        if (m)
            peer_print (p->server, peer, LVL_INFO, m);
        free (m);
        if (ctx_verified (ctx))
            peer_print (p->server, peer, LVL_SUCCESS,
                        _("The peer's identity has been verified."));
        else
            peer_print (p->server, peer, LVL_WARNING,
                        _("You have not verified the peer's identity yet."));
    }
    else
        peer_print (p->server, peer, LVL_WARNING,
                    _("This current conversation is not encrypted."));
    free (account);
    free (peer);
    return WEECHAT_RC_OK;
}

static int
cmd_trust (const struct peer_ref *p, int trust)
{
    char *account, *peer;
    ConnContext *ctx;
    char *m;

    if (!resolve (p, &account, &peer))
        return WEECHAT_RC_ERROR;
    ctx = find_ctx (account, peer);
    if (ctx_encrypted (ctx) && ctx->active_fingerprint)
    {
        otrl_context_set_trust (ctx->active_fingerprint,
                                trust ? "verified" : "");
        otrl_privkey_write_fingerprints (otr_us, otr_fprfile);
        m = strf (trust ? _("%s@%s is now authenticated.")
                        : _("%s@%s is now de-authenticated."), peer, p->server);
        if (m)
            peer_print (p->server, peer, LVL_INFO, m);
        free (m);
        weechat_bar_item_update (OTR_BAR_ITEM);
    }
    else
        peer_print (p->server, peer, LVL_ERROR,
                    _("No active fingerprint. Start an OTR conversation "
                    "first: /otr start"));
    free (account);
    free (peer);
    return WEECHAT_RC_OK;
}

static int
cmd_smp (struct t_gui_buffer *buf, char **tok, int n)
{
    struct peer_ref p;
    char *account = NULL, *peer = NULL;
    const char *secret = NULL, *question = NULL, *action = tok[1];
    ConnContext *ctx;
    int rc = WEECHAT_RC_ERROR;
    struct peer_data *pd;

    if (strcmp (action, "respond") == 0)
    {
        if (n == 3 && peer_args (buf, tok + 2, 0, &p))
            secret = tok[2];
        else if (n == 5 && peer_args (buf, tok + 2, 2, &p))
            secret = tok[4];
        else
            return WEECHAT_RC_ERROR;
    }
    else if (strcmp (action, "ask") == 0)
    {
        if (n == 3 && peer_args (buf, tok + 2, 0, &p))
            secret = tok[2];
        else if (n == 4 && peer_args (buf, tok + 2, 0, &p))
        {
            question = tok[2];
            secret = tok[3];
        }
        else if (n == 5 && peer_args (buf, tok + 2, 2, &p))
            secret = tok[4];
        else if (n == 6 && peer_args (buf, tok + 2, 2, &p))
        {
            question = tok[4];
            secret = tok[5];
        }
        else
            return WEECHAT_RC_ERROR;
    }
    else if (strcmp (action, "abort") == 0)
    {
        if (n == 2 && peer_args (buf, tok + 2, 0, &p))
            ;
        else if (n == 4 && peer_args (buf, tok + 2, 2, &p))
            ;
        else
            return WEECHAT_RC_ERROR;
    }
    else
        return WEECHAT_RC_ERROR;

    if (!resolve (&p, &account, &peer))
        return WEECHAT_RC_ERROR;
    ctx = find_ctx (account, peer);
    if (!ctx_encrypted (ctx))
    {
        char *m = strf (_("There is currently no encrypted session with "
                        "%s@%s."), peer, p.server);

        if (m)
            peer_print (p.server, peer, LVL_ERROR, m);
        free (m);
        rc = WEECHAT_RC_OK;
        goto done;
    }
    pd = peer_data_for (account, peer);

    if (strcmp (action, "respond") == 0)
        otrl_message_respond_smp (otr_us, &otr_ops, NULL, ctx,
                                  (const unsigned char *)secret,
                                  strlen (secret));
    else if (strcmp (action, "ask") == 0)
    {
        if (question)
        {
            otrl_message_initiate_smp_q (otr_us, &otr_ops, NULL, ctx, question,
                                         (const unsigned char *)secret,
                                         strlen (secret));
            peer_print (p.server, peer, LVL_INFO, _("SMP question sent..."));
        }
        else
        {
            otrl_message_initiate_smp (otr_us, &otr_ops, NULL, ctx,
                                       (const unsigned char *)secret,
                                       strlen (secret));
            peer_print (p.server, peer, LVL_INFO, _("SMP challenge sent..."));
        }
        if (pd)
            pd->in_smp = 1;
    }
    else
    {
        otrl_message_abort_smp (otr_us, &otr_ops, NULL, ctx);
        if (pd)
        {
            pd->in_smp = 0;
            pd->smp_question = 0;
        }
        peer_print (p.server, peer, LVL_INFO, _("SMP aborted."));
    }
    rc = WEECHAT_RC_OK;
done:
    free (account);
    free (peer);
    return rc;
}

static int
cmd_log (struct t_gui_buffer *buf, char **tok, int n)
{
    struct peer_ref p;
    char *account, *peer;
    ConnContext *ctx;
    struct peer_data *pd;

    if (!peer_args (buf, tok + 1, 0, &p))
    {
        otr_print_str (NULL, LVL_ERROR, _("OTR LOG: Not in an OTR session"));
        return WEECHAT_RC_OK;
    }
    if (!resolve (&p, &account, &peer))
        return WEECHAT_RC_ERROR;
    ctx = find_ctx (account, peer);
    pd = peer_data_for (account, peer);

    if (!ctx_encrypted (ctx))
        peer_print (p.server, peer, LVL_ERROR, _("OTR LOG: Not in an OTR session"));
    else if (n == 1)
    {
        if (buf_is_logged (buf))
            peer_print (p.server, peer, LVL_WARNING,
                        _("This conversation is currently being logged."));
        else
            peer_print (p.server, peer, LVL_INFO,
                        _("This conversation is currently NOT being logged."));
    }
    else if (strcmp (tok[1], "start") == 0 && !buf_is_logged (buf))
    {
        if (pd && pd->prev_log == LOG_UNTOUCHED)
            pd->prev_log = buf_log_level (buf);
        peer_print (p.server, peer, LVL_WARNING,
                    _("From this point on, this conversation will be logged. "
                    "Please keep in mind that by doing so you are "
                    "potentially putting yourself and your interlocutor at "
                    "risk. You can disable this by doing /otr log stop"));
        weechat_command (buf, "/mute logger set 9");
    }
    else if (strcmp (tok[1], "stop") == 0 && buf_is_logged (buf))
    {
        if (pd && pd->prev_log == LOG_UNTOUCHED)
            pd->prev_log = buf_log_level (buf);
        weechat_command (buf, "/mute logger set 0");
        peer_print (p.server, peer, LVL_INFO,
                    _("From this point on, this conversation will NOT be "
                    "logged ANYMORE."));
    }
    weechat_bar_item_update (OTR_BAR_ITEM);
    free (account);
    free (peer);
    return WEECHAT_RC_OK;
}

static int
policy_known (const char *name)
{
    int i;

    for (i = 0; i < N_POLICIES; i++)
        if (strcasecmp (policies[i].name, name) == 0)
            return 1;
    return 0;
}

static int
value_ok (const char *v)
{
    return strcasecmp (v, "on") == 0 || strcasecmp (v, "off") == 0
        || strcasecmp (v, "toggle") == 0;
}

static void
print_policies_here (struct t_gui_buffer *buf, const char *text)
{
    struct peer_ref p;
    char *account, *peer;

    if (peer_args (buf, NULL, 0, &p) && resolve (&p, &account, &peer))
    {
        peer_print (p.server, peer, LVL_INFO, text);
        free (account);
        free (peer);
    }
    else
        otr_print_str (NULL, LVL_INFO, text);
}

static int
cmd_policy (struct t_gui_buffer *buf, char **tok, int n)
{
    struct peer_ref p;
    char *account, *peer, *text, *cmd;

    if (n == 1 || (n == 2 && strcasecmp (tok[1], "default") == 0))
    {
        if (n == 1 && peer_args (buf, NULL, 0, &p)
            && resolve (&p, &account, &peer))
        {
            text = format_peer_policies (account, peer);
            if (text)
                peer_print (p.server, peer, LVL_INFO, text);
            free (text);
            free (account);
            free (peer);
        }
        else
        {
            text = format_default_policies ();
            if (text)
                print_policies_here (buf, text);
            free (text);
        }
        return WEECHAT_RC_OK;
    }
    if (n == 3 && policy_known (tok[1]) && value_ok (tok[2]))
    {
        if (!peer_args (buf, NULL, 0, &p) || !resolve (&p, &account, &peer))
            return WEECHAT_RC_ERROR;
        {
            char nick[128];

            account_nick (account, nick, sizeof (nick));
            cmd = strf ("/set otr.policy.%s.%s.%s.%s %s", p.server, nick,
                        peer, tok[1], tok[2]);
        }
        if (cmd)
            weechat_command (NULL, cmd);
        free (cmd);
        text = format_peer_policies (account, peer);
        if (text)
            peer_print (p.server, peer, LVL_INFO, text);
        free (text);
        free (account);
        free (peer);
        return WEECHAT_RC_OK;
    }
    if (n == 4 && strcasecmp (tok[1], "default") == 0 && policy_known (tok[2])
        && value_ok (tok[3]))
    {
        cmd = strf ("/set otr.policy.default.%s %s", tok[2], tok[3]);
        if (cmd)
            weechat_command (NULL, cmd);
        free (cmd);
        text = format_default_policies ();
        if (text)
            print_policies_here (buf, text);
        free (text);
        return WEECHAT_RC_OK;
    }
    return WEECHAT_RC_ERROR;
}

static int
command_cb (const void *pointer, void *data, struct t_gui_buffer *buffer,
            int argc, char **argv, char **argv_eol)
{
    char *tok[16];
    int n = 0, i, rc = WEECHAT_RC_ERROR;
    struct peer_ref p;
    const char *c;

    (void)pointer; (void)data; (void)argv;

    if (argc > 1)
        n = split_args (argv_eol[1], tok, 16);
    if (n <= 0)
    {
        otr_error_core (_("invalid arguments, see /help otr"));
        return WEECHAT_RC_ERROR;
    }
    c = tok[0];

    if ((n == 1 || n == 3)
        && (strcmp (c, "start") == 0 || strcmp (c, "refresh") == 0))
    {
        if (peer_args (buffer, tok + 1, n - 1, &p))
            rc = cmd_start (buffer, &p);
    }
    else if ((n == 1 || n == 3)
             && (strcmp (c, "finish") == 0 || strcmp (c, "end") == 0))
    {
        if (peer_args (buffer, tok + 1, n - 1, &p))
            rc = cmd_finish (&p);
    }
    else if ((n == 1 || n == 3) && strcmp (c, "status") == 0)
    {
        if (peer_args (buffer, tok + 1, n - 1, &p))
            rc = cmd_status (&p);
    }
    else if (n >= 2 && n <= 6 && strcmp (c, "smp") == 0)
        rc = cmd_smp (buffer, tok, n);
    else if ((n == 1 || n == 3)
             && (strcmp (c, "trust") == 0 || strcmp (c, "distrust") == 0))
    {
        if (peer_args (buffer, tok + 1, n - 1, &p))
            rc = cmd_trust (&p, strcmp (c, "trust") == 0);
    }
    else if ((n == 1 || n == 2) && strcmp (c, "log") == 0)
        rc = cmd_log (buffer, tok, n);
    else if (n >= 1 && n <= 4 && strcmp (c, "policy") == 0)
        rc = cmd_policy (buffer, tok, n);
    else if ((n == 1 || n == 2) && strcmp (c, "fingerprint") == 0)
    {
        if (n == 1)
            show_account_fingerprints ();
        else if (strcmp (tok[1], "all") == 0)
            show_peer_fingerprints (NULL);
        else
            show_peer_fingerprints (tok[1]);
        rc = WEECHAT_RC_OK;
    }

    if (rc == WEECHAT_RC_ERROR)
        otr_error_core (_("invalid arguments or not in a private buffer, see "
                        "/help otr"));
    for (i = 0; i < n; i++)
        free (tok[i]);
    return rc;
}

static int
policy_completion_cb (const void *pointer, void *data,
                      const char *completion_item,
                      struct t_gui_buffer *buffer,
                      struct t_gui_completion *completion)
{
    int i;

    (void)pointer; (void)data; (void)completion_item; (void)buffer;
    for (i = 0; i < N_POLICIES; i++)
        weechat_hook_completion_list_add (completion, policies[i].name, 0,
                                          WEECHAT_LIST_POS_SORT);
    return WEECHAT_RC_OK;
}

static int
logger_level_update_cb (const void *pointer, void *data, const char *option,
                        const char *value)
{
    (void)pointer; (void)data; (void)option; (void)value;
    weechat_bar_item_update (OTR_BAR_ITEM);
    return WEECHAT_RC_OK;
}

static int
buffer_switch_cb (const void *pointer, void *data, const char *signal,
                  const char *type_data, void *signal_data)
{
    (void)pointer; (void)data; (void)signal; (void)type_data;
    (void)signal_data;
    weechat_bar_item_update (OTR_BAR_ITEM);
    return WEECHAT_RC_OK;
}

/* end the OTR session when its private buffer is closed */
static int
buffer_closing_cb (const void *pointer, void *data, const char *signal,
                   const char *type_data, void *signal_data)
{
    struct t_gui_buffer *buf = signal_data;
    struct peer_ref p;
    char *account, *peer;

    (void)pointer; (void)data; (void)signal; (void)type_data;
    if (peer_args (buf, NULL, 0, &p) && resolve (&p, &account, &peer))
    {
        ConnContext *ctx = find_ctx (account, peer);

        if (ctx && ctx->msgstate != OTRL_MSGSTATE_PLAINTEXT)
            otrl_message_disconnect_all_instances (otr_us, &otr_ops, NULL,
                                                   account, OTR_PROTOCOL,
                                                   peer);
        free (account);
        free (peer);
    }
    return WEECHAT_RC_OK;
}

/* ------------------------------------------------------------------------ */
/* plugin init / end                                                         */
/* ------------------------------------------------------------------------ */

static int
file_exists (const char *path)
{
    struct stat st;

    return stat (path, &st) == 0;
}

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    const char *base;

    Dl_info dli;

    (void)argc; (void)argv;
    weechat_plugin = plugin;

    /*
     * libotr registers handlers inside libgcrypt. If WeeChat dlclose()s this
     * plugin (and with it libotr) on /plugin unload, libgcrypt keeps calling
     * into unmapped memory and WeeChat crashes later. Pin the plugin and its
     * dependencies in memory. Consequence: a rebuilt otr.so is only picked up
     * after restarting WeeChat, and static state must be reset on every init.
     */
    if (dladdr ((void *)weechat_plugin_init, &dli) && dli.dli_fname)
        dlopen (dli.dli_fname, RTLD_NOW | RTLD_NODELETE);
    /* the dependency libotr is not pinned by the line above: do it directly */
    if (dladdr ((void *)otrl_init, &dli) && dli.dli_fname)
        dlopen (dli.dli_fname, RTLD_NOW | RTLD_NODELETE);

    g_shutdown = 0;
    g_injecting = 0;
    g_no_tag = 0;
    otr_us = NULL;
    otr_bar_item = NULL;

    if (otrl_init (OTRL_VERSION_MAJOR, OTRL_VERSION_MINOR, OTRL_VERSION_SUB))
    {
        weechat_printf (NULL, "%sotr: libotr version mismatch",
                        weechat_prefix ("error"));
        return WEECHAT_RC_ERROR;
    }

    if (!config_init ())
    {
        weechat_printf (NULL, "%sotr: could not create configuration",
                        weechat_prefix ("error"));
        return WEECHAT_RC_ERROR;
    }

    base = weechat_info_get ("weechat_data_dir", "");
    if (!base || !*base)
        base = weechat_info_get ("weechat_dir", "");
    if (!base || !*base)
        return WEECHAT_RC_ERROR;
    otr_dir = strf ("%s/otr", base);
    otr_keyfile = strf ("%s/otr.private_key", otr_dir);
    otr_fprfile = strf ("%s/otr.fingerprints", otr_dir);
    otr_instagfile = strf ("%s/otr.instag", otr_dir);
    if (!otr_dir || !otr_keyfile || !otr_fprfile || !otr_instagfile)
        return WEECHAT_RC_ERROR;
    weechat_mkdir_parents (otr_dir, 0700);

    otr_us = otrl_userstate_create ();
    if (!otr_us)
        return WEECHAT_RC_ERROR;
    if (file_exists (otr_keyfile))
        otrl_privkey_read (otr_us, otr_keyfile);
    if (file_exists (otr_fprfile))
        otrl_privkey_read_fingerprints (otr_us, otr_fprfile, NULL, NULL);
    if (file_exists (otr_instagfile))
        otrl_instag_read (otr_us, otr_instagfile);

    weechat_hook_modifier ("irc_in_privmsg", &modifier_in_cb, NULL, NULL);
    weechat_hook_modifier ("irc_out_privmsg", &modifier_out_cb, NULL, NULL);

    weechat_hook_command (
        "otr",
        N_("Off-the-Record (OTR) encryption for private IRC conversations"),
        N_("start|refresh [<nick> <server>]"
            " || finish|end [<nick> <server>]"
            " || status [<nick> <server>]"
            " || trust|distrust [<nick> <server>]"
            " || smp ask [<nick> <server>] [<question>] <secret>"
            " || smp respond [<nick> <server>] <secret>"
            " || smp abort [<nick> <server>]"
            " || log [start|stop]"
            " || policy [default] [<name> on|off|toggle]"
            " || fingerprint [<search>|all]"),
        N_("      start: start an OTR session with a peer (sends an OTR query)\n"
            "    refresh: refresh the OTR session (same as start)\n"
            "     finish: end the OTR session\n"
            "        end: same as finish\n"
            "     status: show whether the conversation is encrypted and authenticated, with both fingerprints\n"
            "      trust: mark the current fingerprint of the peer as verified\n"
            "   distrust: remove the verified mark from the current fingerprint of the peer\n"
            "        smp: authenticate the peer with the Socialist Millionaire Protocol (SMP):\n"
            "                 ask: send a shared secret, optionally with a question\n"
            "             respond: answer the request of the peer\n"
            "               abort: abort the running verification\n"
            "        log: show or change logging of the current OTR session; the previous setting is restored when the session ends\n"
            "     policy: show or change OTR policies, for the peer of the current private buffer or, with \"default\", for all peers\n"
            "fingerprint: show your own fingerprints (without argument), those of all known peers (all) or of the peers matching <search>\n"
            "\n"
            "    nick: nick of the peer (default: peer of the current private buffer)\n"
            "  server: internal name of the IRC server (default: server of the current private buffer)\n"
            "question: question that the peer has to answer\n"
            "  secret: secret that both you and the peer know\n"
            "    name: name of the policy\n"
            "  search: only show peers whose nick contains this text\n"
            "\n"
            "Policies:\n"
            "            allow_v2: allow OTR protocol version 2\n"
            "            allow_v3: allow OTR protocol version 3\n"
            "  require_encryption: refuse to send unencrypted messages when OTR is enabled\n"
            "                 log: enable logging of OTR conversations\n"
            "            send_tag: advertise your OTR capability using the whitespace tag\n"
            "         html_escape: escape HTML special characters in outbound messages\n"
            "         html_filter: filter HTML in incoming messages\n"
            "\n"
            "Note: OTR works for private conversations only, not for channels.\n"
            "Note: to show the OTR state in the status bar, add \"[otr]\" to the option weechat.bar.status.items; use /set otr.* to customize the item and the colors.\n"
            "Note: policies are stored in the options otr.policy.*; a more specific option (peer) overrides a more general one (server, default).\n"
            "\n"
            "Examples:\n"
            "  start an OTR session in the current private buffer:\n"
            "    /otr start\n"
            "  verify the peer with a shared secret:\n"
            "    /otr smp ask \"swordfish\"\n"
            "  verify the peer with a question and a shared secret:\n"
            "    /otr smp ask \"Where did we meet?\" \"Berlin\"\n"
            "  answer an SMP request of the peer:\n"
            "    /otr smp respond \"Berlin\"\n"
            "  mark the fingerprint as verified after comparing it over a secure channel:\n"
            "    /otr trust\n"
            "  show the policies of the current peer and enable the whitespace tag for it:\n"
            "    /otr policy\n"
            "    /otr policy send_tag on\n"
            "  enable the whitespace tag for all peers:\n"
            "    /otr policy default send_tag on\n"
            "  show the fingerprints of all known peers:\n"
            "    /otr fingerprint all\n"
            "  end the OTR session:\n"
            "    /otr finish"),
        "start|refresh|finish|end|status|trust|distrust %(nick) %(irc_servers) %-"
        " || smp ask|respond|abort %(nick) %(irc_servers) %-"
        " || log start|stop %-"
        " || policy default %(otr_policy) on|off|toggle %-"
        " || policy %(otr_policy) on|off|toggle %-"
        " || fingerprint all %-",
        &command_cb, NULL, NULL);

    weechat_hook_completion ("otr_policy", N_("OTR policies"),
                             &policy_completion_cb, NULL, NULL);
    weechat_hook_config ("logger.level.irc.*", &logger_level_update_cb, NULL,
                         NULL);
    weechat_hook_signal ("buffer_switch", &buffer_switch_cb, NULL, NULL);
    weechat_hook_signal ("buffer_closing", &buffer_closing_cb, NULL, NULL);

    otr_bar_item = weechat_bar_item_new (OTR_BAR_ITEM, &bar_item_cb, NULL,
                                         NULL);
    weechat_bar_item_update (OTR_BAR_ITEM);

    return WEECHAT_RC_OK;
}

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    ConnContext *c;
    char **accts = NULL, **users = NULL, **t1, **t2;
    int n = 0, cap = 0, i;

    (void)plugin;
    g_shutdown = 1;

    if (otr_us)
    {
        /* end all sessions politely. Collect names first: the callbacks
         * must not run while we walk libotr's context list. */
        for (c = otr_us->context_root; c; c = c->next)
        {
            if (c->msgstate == OTRL_MSGSTATE_PLAINTEXT)
                continue;
            if (n == cap)
            {
                cap = cap ? cap * 2 : 8;
                t1 = realloc (accts, (size_t)cap * sizeof (char *));
                if (!t1)
                    break;
                accts = t1;
                t2 = realloc (users, (size_t)cap * sizeof (char *));
                if (!t2)
                    break;
                users = t2;
            }
            accts[n] = strdup (c->accountname);
            users[n] = strdup (c->username);
            n++;
        }
        for (i = 0; i < n; i++)
        {
            if (accts[i] && users[i])
                otrl_message_disconnect_all_instances (
                    otr_us, &otr_ops, NULL, accts[i], OTR_PROTOCOL, users[i]);
            free (accts[i]);
            free (users[i]);
        }
        free (accts);
        free (users);
        pending_clear_all ();
        otrl_privkey_write_fingerprints (otr_us, otr_fprfile);
        otrl_userstate_free (otr_us);
        otr_us = NULL;
    }
    if (otr_bar_item)
    {
        weechat_bar_item_remove (otr_bar_item);
        otr_bar_item = NULL;
    }
    if (cfg_file)
    {
        weechat_config_write (cfg_file);
        weechat_config_free (cfg_file);
        cfg_file = NULL;
    }
    free (otr_dir);
    free (otr_keyfile);
    free (otr_fprfile);
    free (otr_instagfile);
    otr_dir = otr_keyfile = otr_fprfile = otr_instagfile = NULL;
    return WEECHAT_RC_OK;
}
