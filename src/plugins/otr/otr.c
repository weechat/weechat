/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * otr.c - Off-the-Record (OTR) messaging for WeeChat, written in C on top of
 *         libotr.
 *
 * C port of the feature set of the Python script "otr.py" (weechat-otr by
 * Matthew M. Boedicker, Nils Goers, Daniel Faucon, Felix Eckhofer), which
 * was built on the "potr" library. This plugin uses libotr instead, so it
 * speaks OTR protocol versions 2 and 3.
 *
 * DISCLAIMER: no guarantee of any kind. Review before relying on it.
 */

#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-bar-item.h"
#include "otr-buffer.h"
#include "otr-callback.h"
#include "otr-command.h"
#include "otr-completion.h"
#include "otr-config.h"
#include "otr-echo.h"
#include "otr-log.h"
#include "otr-modifier.h"
#include "otr-util.h"


WEECHAT_PLUGIN_NAME(OTR_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Off-the-Record messaging for IRC (libotr)"));
WEECHAT_PLUGIN_AUTHOR("otr-c");
WEECHAT_PLUGIN_VERSION("0.1.0");
WEECHAT_PLUGIN_LICENSE("GPL3");
WEECHAT_PLUGIN_PRIORITY(OTR_PLUGIN_PRIORITY);

struct t_weechat_plugin *weechat_otr_plugin = NULL;

OtrlUserState otr_us = NULL;
char *otr_dir = NULL;
char *otr_keyfile = NULL;
char *otr_fprfile = NULL;
char *otr_instagfile = NULL;

struct t_gui_bar_item *otr_bar_item = NULL;

/* >0 while we inject our own protocol messages through /quote, so that the
 * outgoing modifier lets them through untouched */
int otr_injecting = 0;
/* >0 while sending CTCP, which must not receive a whitespace tag */
int otr_no_tag = 0;
/* set while the plugin unloads: no UI work from libotr callbacks */
int otr_shutdown = 0;

int
otr_file_exists (const char *path)
{
    struct stat st;

    return stat (path, &st) == 0;
}

/*
 * Display infos about external libraries used.
 */

int
otr_debug_libs_cb (const void *pointer, void *data, const char *signal,
                   const char *type_data, void *signal_data)
{
    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    weechat_printf (NULL, "  %s: libotr %s", OTR_PLUGIN_NAME, OTRL_VERSION);

    return WEECHAT_RC_OK;
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

    otr_shutdown = 0;
    otr_injecting = 0;
    otr_no_tag = 0;
    otr_us = NULL;
    otr_bar_item = NULL;

    if (otrl_init (OTRL_VERSION_MAJOR, OTRL_VERSION_MINOR, OTRL_VERSION_SUB))
    {
        weechat_printf (NULL, "%sotr: libotr version mismatch",
                        weechat_prefix ("error"));
        return WEECHAT_RC_ERROR;
    }

    if (!otr_config_init ())
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
    otr_dir = otr_util_strf ("%s/otr", base);
    otr_keyfile = otr_util_strf ("%s/otr.private_key", otr_dir);
    otr_fprfile = otr_util_strf ("%s/otr.fingerprints", otr_dir);
    otr_instagfile = otr_util_strf ("%s/otr.instag", otr_dir);
    if (!otr_dir || !otr_keyfile || !otr_fprfile || !otr_instagfile)
        return WEECHAT_RC_ERROR;
    weechat_mkdir_parents (otr_dir, 0700);

    otr_us = otrl_userstate_create ();
    if (!otr_us)
        return WEECHAT_RC_ERROR;
    if (otr_file_exists (otr_keyfile))
        otrl_privkey_read (otr_us, otr_keyfile);
    if (otr_file_exists (otr_fprfile))
        otrl_privkey_read_fingerprints (otr_us, otr_fprfile, NULL, NULL);
    if (otr_file_exists (otr_instagfile))
        otrl_instag_read (otr_us, otr_instagfile);

    weechat_hook_modifier ("irc_in_privmsg", &otr_modifier_in_cb, NULL, NULL);
    weechat_hook_modifier ("irc_out_privmsg", &otr_modifier_out_cb, NULL, NULL);

    otr_command_init ();

    otr_completion_init ();
    weechat_hook_config ("logger.level.irc.*", &otr_log_logger_level_update_cb,
                         NULL,
                         NULL);
    weechat_hook_signal ("buffer_switch", &otr_buffer_switch_cb, NULL, NULL);
    weechat_hook_signal ("buffer_closing", &otr_buffer_closing_cb, NULL, NULL);
    weechat_hook_signal ("debug_libs", &otr_debug_libs_cb, NULL, NULL);

    otr_bar_item = weechat_bar_item_new (OTR_BAR_ITEM, &otr_bar_item_cb, NULL,
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
    otr_shutdown = 1;

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
                    otr_us, &otr_callback_ops, NULL, accts[i], OTR_PROTOCOL,
                    users[i]);
            free (accts[i]);
            free (users[i]);
        }
        free (accts);
        free (users);
        otr_echo_pending_clear_all ();
        otrl_privkey_write_fingerprints (otr_us, otr_fprfile);
        otrl_userstate_free (otr_us);
        otr_us = NULL;
    }
    if (otr_bar_item)
    {
        weechat_bar_item_remove (otr_bar_item);
        otr_bar_item = NULL;
    }
    if (otr_config_file)
    {
        weechat_config_write (otr_config_file);
        weechat_config_free (otr_config_file);
        otr_config_file = NULL;
    }
    free (otr_dir);
    free (otr_keyfile);
    free (otr_fprfile);
    free (otr_instagfile);
    otr_dir = otr_keyfile = otr_fprfile = otr_instagfile = NULL;
    return WEECHAT_RC_OK;
}
