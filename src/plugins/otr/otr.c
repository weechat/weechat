/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Off-the-Record (OTR) Messaging protocol for WeeChat */

/*
 * This plugin is based on the Python script "otr.py"
 * (https://github.com/mmb/weechat-otr), written by:
 *   - Matthew M. Boedicker <matthewm@boedicker.org>
 *   - Nils Görs <weechatter@arcor.de>
 *   - Daniel "koolfy" Faucon <koolfy@koolfy.be>
 *   - Felix Eckhofer <felix@tribut.de>
 *
 * Only the IRC plugin is supported for now (private messages), and the OTR
 * protocol versions 2 and 3 are supported.
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
#include "otr-command.h"
#include "otr-completion.h"
#include "otr-config.h"
#include "otr-context.h"
#include "otr-echo.h"
#include "otr-irc.h"
#include "otr-log.h"
#include "otr-modifier.h"


WEECHAT_PLUGIN_NAME(OTR_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Off-the-Record Messaging protocol"));
WEECHAT_PLUGIN_AUTHOR("Nils Görs <weechatter@arcor.de>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(OTR_PLUGIN_PRIORITY);

struct t_weechat_plugin *weechat_otr_plugin = NULL;

OtrlUserState otr_us = NULL;
char *otr_dir = NULL;
char *otr_keyfile = NULL;
char *otr_fprfile = NULL;
char *otr_instagfile = NULL;

struct t_gui_bar_item *otr_bar_item = NULL;

/*
 * >0 while we inject our own protocol messages through /quote, so that the
 * outgoing modifier lets them through untouched.
 */
int otr_injecting = 0;

/* >0 while sending CTCP, which must not receive a whitespace tag. */
int otr_no_tag = 0;

/* Set while the plugin unloads: no UI work from libotr callbacks. */
int otr_shutdown = 0;


/*
 * Check if a file exists.
 *
 * Return 1 if the file exists, 0 otherwise.
 */

int
otr_file_exists (const char *path)
{
    struct stat st;

    return (stat (path, &st) == 0) ? 1 : 0;
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

/*
 * Initialize OTR plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    char *data_dir;
    Dl_info dl_info;

    /* Make C compiler happy. */
    (void) argc;
    (void) argv;

    weechat_plugin = plugin;

    /*
     * libotr registers handlers inside libgcrypt. If WeeChat dlclose()s this
     * plugin (and with it libotr) on /plugin unload, libgcrypt keeps calling
     * into unmapped memory and WeeChat crashes later. Pin the plugin and its
     * dependencies in memory. Consequence: a rebuilt otr.so is only picked up
     * after restarting WeeChat, and static state must be reset on every init.
     */
    if (dladdr ((void *)weechat_plugin_init, &dl_info) && dl_info.dli_fname)
        dlopen (dl_info.dli_fname, RTLD_NOW | RTLD_NODELETE);
    /* The dependency libotr is not pinned by the line above: do it directly. */
    if (dladdr ((void *)otrl_init, &dl_info) && dl_info.dli_fname)
        dlopen (dl_info.dli_fname, RTLD_NOW | RTLD_NODELETE);

    otr_shutdown = 0;
    otr_injecting = 0;
    otr_no_tag = 0;
    otr_us = NULL;
    otr_bar_item = NULL;

    if (otrl_init (OTRL_VERSION_MAJOR, OTRL_VERSION_MINOR, OTRL_VERSION_SUB))
    {
        weechat_printf (NULL, "%s%s: libotr version mismatch",
                        weechat_prefix ("error"), OTR_PLUGIN_NAME);
        return WEECHAT_RC_ERROR;
    }

    if (!otr_config_init ())
    {
        weechat_printf (NULL, "%s%s: could not create configuration",
                        weechat_prefix ("error"), OTR_PLUGIN_NAME);
        return WEECHAT_RC_ERROR;
    }

    data_dir = weechat_info_get ("weechat_data_dir", "");
    if (!data_dir || !data_dir[0])
    {
        free (data_dir);
        data_dir = weechat_info_get ("weechat_dir", "");
    }
    if (!data_dir || !data_dir[0])
    {
        free (data_dir);
        return WEECHAT_RC_ERROR;
    }
    weechat_asprintf (&otr_dir, "%s/otr", data_dir);
    free (data_dir);
    if (!otr_dir)
        return WEECHAT_RC_ERROR;
    weechat_asprintf (&otr_keyfile, "%s/otr.private_key", otr_dir);
    weechat_asprintf (&otr_fprfile, "%s/otr.fingerprints", otr_dir);
    weechat_asprintf (&otr_instagfile, "%s/otr.instag", otr_dir);
    if (!otr_keyfile || !otr_fprfile || !otr_instagfile)
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

    weechat_hook_config ("logger.level.irc.*",
                         &otr_log_logger_level_update_cb, NULL, NULL);
    weechat_hook_signal ("buffer_switch", &otr_buffer_switch_cb, NULL, NULL);
    weechat_hook_signal ("irc_pv_opened", &otr_buffer_irc_pv_opened_cb,
                         NULL, NULL);
    weechat_hook_signal ("buffer_closing", &otr_buffer_closing_cb, NULL, NULL);
    weechat_hook_signal ("debug_libs", &otr_debug_libs_cb, NULL, NULL);
    weechat_hook_signal ("*,irc_in2_nick", &otr_irc_nick_cb, NULL, NULL);

    otr_bar_item = weechat_bar_item_new (OTR_BAR_ITEM,
                                         &otr_bar_item_cb, NULL, NULL);
    weechat_bar_item_update (OTR_BAR_ITEM);

    return WEECHAT_RC_OK;
}

/*
 * End OTR plugin: end all OTR sessions, save fingerprints and configuration,
 * free data.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    ConnContext *ptr_context;
    char **accounts, **usernames, **new_accounts, **new_usernames;
    int num_sessions, size_alloc, i;

    /* Make C compiler happy. */
    (void) plugin;

    otr_shutdown = 1;

    if (otr_us)
    {
        /*
         * End all sessions politely. Collect names first: the callbacks
         * must not run while we walk libotr's context list.
         */
        accounts = NULL;
        usernames = NULL;
        num_sessions = 0;
        size_alloc = 0;
        for (ptr_context = otr_us->context_root; ptr_context;
             ptr_context = ptr_context->next)
        {
            if (ptr_context->msgstate == OTRL_MSGSTATE_PLAINTEXT)
                continue;
            if (num_sessions == size_alloc)
            {
                size_alloc = (size_alloc) ? size_alloc * 2 : 8;
                new_accounts = realloc (accounts,
                                        (size_t)size_alloc * sizeof (char *));
                if (!new_accounts)
                    break;
                accounts = new_accounts;
                new_usernames = realloc (usernames,
                                         (size_t)size_alloc * sizeof (char *));
                if (!new_usernames)
                    break;
                usernames = new_usernames;
            }
            accounts[num_sessions] = strdup (ptr_context->accountname);
            usernames[num_sessions] = strdup (ptr_context->username);
            num_sessions++;
        }
        for (i = 0; i < num_sessions; i++)
        {
            if (accounts[i] && usernames[i])
                otr_context_end_session (accounts[i], usernames[i], 0, 0);
            free (accounts[i]);
            free (usernames[i]);
        }
        free (accounts);
        free (usernames);

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
    otr_config_free_regex_no_send_tag ();

    free (otr_dir);
    otr_dir = NULL;
    free (otr_keyfile);
    otr_keyfile = NULL;
    free (otr_fprfile);
    otr_fprfile = NULL;
    free (otr_instagfile);
    otr_instagfile = NULL;

    return WEECHAT_RC_OK;
}
