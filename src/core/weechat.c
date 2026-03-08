/*
 * ##########################################################################
 * ##          ___       __         ______________        _____            ##
 * ##          __ |     / /___________  ____/__  /_______ __  /_           ##
 * ##          __ | /| / /_  _ \  _ \  /    __  __ \  __ `/  __/           ##
 * ##          __ |/ |/ / /  __/  __/ /___  _  / / / /_/ // /_             ##
 * ##          ____/|__/  \___/\___/\____/  /_/ /_/\__,_/ \__/             ##
 * ##                                                                      ##
 * ##             WeeChat - Wee Enhanced Environment for Chat              ##
 * ##                 Fast, light, extensible chat client                  ##
 * ##                                                                      ##
 * ##             By Sébastien Helleu <flashcode@flashtux.org>             ##
 * ##                                                                      ##
 * ##                         https://weechat.org/                         ##
 * ##                                                                      ##
 * ##########################################################################
 *
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

/* WeeChat main functions */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>

#ifdef HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif

#include "weechat.h"
#include "core-args.h"
#include "core-command.h"
#include "core-completion.h"
#include "core-config.h"
#include "core-debug.h"
#include "core-dir.h"
#include "core-doc.h"
#include "core-eval.h"
#include "core-hdata.h"
#include "core-hook.h"
#include "core-list.h"
#include "core-log.h"
#include "core-network.h"
#include "core-proxy.h"
#include "core-secure.h"
#include "core-secure-config.h"
#include "core-signal.h"
#include "core-string.h"
#include "core-upgrade.h"
#include "core-url.h"
#include "core-utf8.h"
#include "core-util.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-completion.h"
#include "../gui/gui-focus.h"
#include "../gui/gui-key.h"
#include "../gui/gui-layout.h"
#include "../gui/gui-main.h"
#include "../plugins/plugin.h"
#include "../plugins/plugin-api.h"

int weechat_headless = 0;              /* 1 if running headless (no GUI)    */
int weechat_daemon = 0;                /* 1 if daemonized (no foreground)   */
int weechat_log_stdout = 0;            /* 1 to log messages on stdout       */
int weechat_debug_core = 0;            /* debug level for core              */
char *weechat_argv0 = NULL;            /* WeeChat binary file name (argv[0])*/
int weechat_upgrading = 0;             /* =1 if WeeChat is upgrading        */
int weechat_first_start = 0;           /* first start of WeeChat?           */
time_t weechat_first_start_time = 0;   /* start time (used by /uptime cmd)  */
int weechat_upgrade_count = 0;         /* number of /upgrade done           */
struct timeval weechat_current_start_timeval; /* start time used to display */
                                       /* duration of /upgrade              */
int weechat_quit = 0;                  /* = 1 if quit request from user     */
volatile sig_atomic_t weechat_quit_signal = 0; /* signal received,          */
                                       /* WeeChat must quit                 */
volatile sig_atomic_t weechat_reload_signal = 0; /* signal received,        */
                                       /* WeeChat must reload configuration */
char *weechat_home_force = NULL;       /* forced home (with -d/--dir)       */
int weechat_home_temp = 0;             /* 1 if using a temporary home       */
int weechat_home_delete_on_exit = 0;   /* 1 if home is deleted on exit      */
char *weechat_config_dir = NULL;       /* config directory                  */
char *weechat_data_dir = NULL;         /* data directory                    */
char *weechat_state_dir = NULL;        /* state directory                   */
char *weechat_cache_dir = NULL;        /* cache directory                   */
char *weechat_runtime_dir = NULL;      /* runtime directory                 */
int weechat_locale_ok = 0;             /* is locale OK?                     */
char *weechat_local_charset = NULL;    /* example: ISO-8859-1, UTF-8        */
int weechat_server_cmd_line = 0;       /* at least 1 server on cmd line     */
char *weechat_force_plugin_autoload = NULL; /* force load of plugins        */
int weechat_doc_gen = 0;               /* doc generation                    */
char *weechat_doc_gen_path = NULL;     /* path for doc generation           */
int weechat_doc_gen_ok = 0;            /* doc generation was successful?    */
int weechat_plugin_no_dlclose = 0;     /* remove calls to dlclose for libs  */
                                       /* (useful with valgrind)            */
int weechat_no_gnutls = 0;             /* remove init/deinit of gnutls      */
                                       /* (useful with valgrind/electric-f.)*/
int weechat_no_gcrypt = 0;             /* remove init/deinit of gcrypt      */
                                       /* (useful with valgrind)            */
struct t_weelist *weechat_startup_commands = NULL; /* startup commands      */
                                                   /* (option -r)           */
int weechat_auto_connect = 1;          /* auto-connect to servers           */
int weechat_auto_load_scripts = 1;     /* auto-load scripts                 */


/*
 * Displays WeeChat startup message.
 */

void
weechat_startup_message (void)
{
    if (weechat_headless)
    {
        string_fprintf (stdout,
                        _("WeeChat is running in headless mode "
                          "(ctrl-c to quit)."));
        string_fprintf (stdout, "\n");
    }

    if (CONFIG_BOOLEAN(config_startup_display_logo))
    {
        gui_chat_printf (
            NULL,
            "%s  ___       __         ______________        _____ \n"
            "%s  __ |     / /___________  ____/__  /_______ __  /_\n"
            "%s  __ | /| / /_  _ \\  _ \\  /    __  __ \\  __ `/  __/\n"
            "%s  __ |/ |/ / /  __/  __/ /___  _  / / / /_/ // /_  \n"
            "%s  ____/|__/  \\___/\\___/\\____/  /_/ /_/\\__,_/ \\__/  ",
            GUI_COLOR(GUI_COLOR_CHAT_NICK),
            GUI_COLOR(GUI_COLOR_CHAT_NICK),
            GUI_COLOR(GUI_COLOR_CHAT_NICK),
            GUI_COLOR(GUI_COLOR_CHAT_NICK),
            GUI_COLOR(GUI_COLOR_CHAT_NICK));
    }
    if (CONFIG_BOOLEAN(config_startup_display_version))
    {
        command_version_display (
            NULL,  /* buffer */
            0,  /* send_to_buffer_as_input */
            0,  /* translated_string */
            0,  /* display_git_version */
            (weechat_upgrade_count > 0) ? 1 : 0);  /* display_upgrades */
    }
    if (CONFIG_BOOLEAN(config_startup_display_logo) ||
        CONFIG_BOOLEAN(config_startup_display_version))
    {
        gui_chat_printf (
            NULL,
            "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    }

    if (weechat_first_start)
    {
        /* message on first run (when weechat.conf is created) */
        gui_chat_printf (NULL, "");
        gui_chat_printf (
            NULL,
            _("Welcome to WeeChat!\n"
              "\n"
              "If you are discovering WeeChat, it is recommended to read at "
              "least the quickstart guide, and the user's guide if you have "
              "some time; they explain main WeeChat concepts.\n"
              "All WeeChat docs are available at: https://weechat.org/doc/\n"
              "\n"
              "Moreover, there is inline help with /help on all commands and "
              "options (use Tab key to complete the name).\n"
              "The command /fset can help to customize WeeChat.\n"
              "\n"
              "You can add and connect to an IRC server with /server and "
              "/connect commands (see /help server)."));
        gui_chat_printf (NULL, "");
        gui_chat_printf (NULL, "---");
        gui_chat_printf (NULL, "");
    }
}

/*
 * Displays warnings about $TERM if it is detected as wrong.
 *
 * If $TERM is different from "screen" or "screen-256color" and that $STY is
 * set (GNU screen) or $TMUX is set (tmux), then a warning is displayed.
 */

void
weechat_term_check (void)
{
    char *term, *sty, *tmux;
    const char *screen_terms = "screen-256color, screen";
    const char *tmux_terms = "tmux-256color, tmux, screen-256color, screen";
    const char *ptr_terms;
    int is_term_ok, is_screen, is_tmux;

    term = getenv ("TERM");
    sty = getenv ("STY");
    tmux = getenv ("TMUX");

    is_screen = (sty && sty[0]);
    is_tmux = (tmux && tmux[0]);

    if (is_screen || is_tmux)
    {
        /* check if $TERM is OK (according to screen/tmux) */
        is_term_ok = 0;
        ptr_terms = NULL;
        if (is_screen)
        {
            is_term_ok = (term && (strncmp (term, "screen", 6) == 0));
            ptr_terms = screen_terms;
        }
        else if (is_tmux)
        {
            is_term_ok = (term
                          && ((strncmp (term, "screen", 6) == 0)
                              || (strncmp (term, "tmux", 4) == 0)));
            ptr_terms = tmux_terms;
        }

        /* display a warning if $TERM is NOT OK */
        if (!is_term_ok)
        {
            gui_chat_printf_date_tags (
                NULL,
                0,
                "term_warning",
                /* TRANSLATORS: the "under %s" can be "under screen" or "under tmux" */
                _("%sWarning: WeeChat is running under %s and $TERM is \"%s\", "
                  "which can cause display bugs; $TERM should be set to one "
                  "of these values: %s"),
                gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                (is_screen) ? "screen" : "tmux",
                (term) ? term : "",
                ptr_terms);
            gui_chat_printf_date_tags (
                NULL,
                0,
                "term_warning",
                _("%sYou should add this line in the file %s:  %s"),
                gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                (is_screen) ? "~/.screenrc" : "~/.tmux.conf",
                (is_screen) ?
                "term screen-256color" :
                "set -g default-terminal \"tmux-256color\"");
        }
    }
}

/*
 * Displays warning about wrong locale ($LANG and $LC_*) if they are detected
 * as wrong.
 */

void
weechat_locale_check (void)
{
    if (!weechat_locale_ok)
    {
        gui_chat_printf (
            NULL,
            _("%sWarning: cannot set the locale; make sure $LANG and $LC_* "
              "variables are correct"),
            gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
    }
}

/*
 * Shutdowns WeeChat.
 */

void
weechat_shutdown (int return_code, int crash)
{
    gui_chat_print_lines_waiting_buffer (stderr);

    log_close ();
    network_end ();
    debug_end ();

    if (!crash && weechat_home_delete_on_exit)
    {
        /* remove temporary home (only if not crashing) */
        dir_remove_home_dirs ();
    }

    free (weechat_argv0);
    free (weechat_home_force);
    free (weechat_config_dir);
    free (weechat_data_dir);
    free (weechat_state_dir);
    free (weechat_cache_dir);
    free (weechat_runtime_dir);
    free (weechat_local_charset);
    free (weechat_force_plugin_autoload);
    weelist_free (weechat_startup_commands);
    free (weechat_doc_gen_path);

    if (crash)
        abort ();
    else if (weechat_doc_gen)
        exit ((weechat_doc_gen_ok) ? 0 : 1);
    else if (return_code >= 0)
        exit (return_code);
}

/*
 * Initializes gettext.
 */

void
weechat_init_gettext (void)
{
    weechat_locale_ok = (setlocale (LC_ALL, "") != NULL);   /* init gettext */
#if ENABLE_NLS == 1
    bindtextdomain (PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (PACKAGE, "UTF-8");
    textdomain (PACKAGE);
#endif /* ENABLE_NLS == 1 */

#ifdef HAVE_LANGINFO_CODESET
    weechat_local_charset = strdup (nl_langinfo (CODESET));
#else
    weechat_local_charset = strdup ("");
#endif /* HAVE_LANGINFO_CODESET */
    utf8_init ();
}

/*
 * Initializes WeeChat.
 */

void
weechat_init (int argc, char *argv[], void (*gui_init_cb)(void))
{
    weechat_first_start_time = time (NULL); /* initialize start time        */
    gettimeofday (&weechat_current_start_timeval, NULL);

    /* set the seed for the pseudo-random integer generator */
    srand ((weechat_current_start_timeval.tv_sec
            * weechat_current_start_timeval.tv_usec)
           ^ getpid ());

    weeurl_init ();                     /* initialize URL                   */
    string_init ();                     /* initialize string                */
    signal_init ();                     /* initialize signals               */
    hdata_init ();                      /* initialize hdata                 */
    hook_init ();                       /* initialize hooks                 */
    debug_init ();                      /* hook signals for debug           */
    gui_color_init ();                  /* initialize colors                */
    gui_chat_init ();                   /* initialize chat                  */
    command_init ();                    /* initialize WeeChat commands      */
    completion_init ();                 /* add core completion hooks        */
    gui_key_init ();                    /* init keys                        */
    network_init_gcrypt ();             /* init gcrypt                      */
    if (!secure_init ())                /* init secured data                */
        weechat_shutdown (EXIT_FAILURE, 0);
    if (!secure_config_init ())         /* init secured data options (sec.*)*/
        weechat_shutdown (EXIT_FAILURE, 0);
    if (!config_weechat_init ())        /* init WeeChat options (weechat.*) */
        weechat_shutdown (EXIT_FAILURE, 0);
    args_parse (argc, argv);            /* parse command line args          */
    dir_create_home_dirs ();            /* create WeeChat home directories  */
    log_init ();                        /* init log file                    */
    plugin_api_init ();                 /* create some hooks (info,hdata,..)*/
    secure_config_read ();              /* read secured data options        */
    config_weechat_read ();             /* read WeeChat options             */
    network_init_gnutls ();             /* init GnuTLS                      */

    if (gui_init_cb)
        (*gui_init_cb) ();              /* init WeeChat interface           */

    if (weechat_upgrading)
    {
        if (upgrade_weechat_load ())    /* upgrade with session file        */
            weechat_upgrade_count++;    /* increase /upgrade count          */
        else
            weechat_upgrading = 0;
    }
    if (!weechat_doc_gen)
        weechat_startup_message ();     /* display WeeChat startup message  */
    gui_chat_print_lines_waiting_buffer (NULL); /* display lines waiting    */
    weechat_term_check ();              /* warning about wrong $TERM        */
    weechat_locale_check ();            /* warning about wrong locale       */
    command_startup (0);                /* command executed before plugins  */
    plugin_init (weechat_force_plugin_autoload, /* init plugin interface(s) */
                 argc, argv);
    command_startup (1);                /* commands executed after plugins  */
    if (!weechat_upgrading)
        gui_layout_window_apply (gui_layout_current, -1);
    if (weechat_upgrading)
        upgrade_weechat_end ();         /* remove .upgrade files + signal   */

    if (weechat_doc_gen)
    {
        weechat_doc_gen_ok = doc_generate (weechat_doc_gen_path);
        weechat_quit = 1;
    }
}

/*
 * Ends WeeChat.
 */

void
weechat_end (void (*gui_end_cb)(int clean_exit))
{
    gui_layout_store_on_exit ();        /* store layout                     */
    plugin_end ();                      /* end plugin interface(s)          */
    if (CONFIG_BOOLEAN(config_look_save_config_on_exit))
        (void) config_weechat_write (); /* save WeeChat config file         */
    (void) secure_config_write ();      /* save secured data                */

    if (gui_end_cb)
        (*gui_end_cb) (1);              /* shut down WeeChat GUI            */

    proxy_free_all ();                  /* free all proxies                 */
    config_weechat_free ();             /* free WeeChat options             */
    secure_config_free ();              /* free secured data options        */
    config_file_free_all ();            /* free all configuration files     */
    gui_key_end ();                     /* remove all keys                  */
    unhook_all ();                      /* remove all hooks                 */
    hdata_end ();                       /* end hdata                        */
    secure_end ();                      /* end secured data                 */
    string_end ();                      /* end string                       */
    weeurl_end ();
    weechat_shutdown (-1, 0);           /* end other things                 */
}
