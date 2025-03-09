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
 * weechat.c - WeeChat main functions
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>

#ifdef HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif

#include "weechat.h"
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
#include "core-version.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-completion.h"
#include "../gui/gui-focus.h"
#include "../gui/gui-key.h"
#include "../gui/gui-layout.h"
#include "../gui/gui-main.h"
#include "../plugins/plugin.h"
#include "../plugins/plugin-api.h"

/* some command line options */
#define OPTION_DOCGEN     1000
#define OPTION_NO_DLCLOSE 1001
#define OPTION_NO_GNUTLS  1002
#define OPTION_NO_GCRYPT  1003

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
 * Displays WeeChat copyright on standard output.
 */

void
weechat_display_copyright (void)
{
    string_fprintf (stdout, "\n");
    string_fprintf (
        stdout,
        /* TRANSLATORS: "%s %s" after "compiled on" is date and time */
        _("WeeChat %s Copyright %s, compiled on %s %s\n"
          "Developed by Sébastien Helleu <flashcode@flashtux.org> "
          "- %s"),
        version_get_version_with_git (),
        WEECHAT_COPYRIGHT_DATE,
        version_get_compilation_date (),
        version_get_compilation_time (),
        WEECHAT_WEBSITE);
    string_fprintf (stdout, "\n");
}

/*
 * Displays WeeChat usage on standard output.
 */

void
weechat_display_usage (void)
{
    weechat_display_copyright ();
    string_fprintf (stdout, "\n");
    string_fprintf (stdout,
                    _("Usage: %s [option...] [plugin:option...]\n"),
                    weechat_argv0);
    string_fprintf (stdout, "\n");
    string_fprintf (
        stdout,
        _("  -a, --no-connect         disable auto-connect to servers at "
          "startup\n"
          "  -c, --colors             display default colors in terminal "
          "and exit\n"
          "  -d, --dir <path>         force a single WeeChat home directory\n"
          "                           or 5 different directories separated "
          "by colons (in this order: config, data, state, cache, runtime)\n"
          "                           (environment variable WEECHAT_HOME is "
          "read if this option is not given)\n"
          "  -t, --temp-dir           create a temporary WeeChat home "
          "directory and delete it on exit\n"
          "                           (incompatible with option \"-d\")\n"
          "  -h, --help               display this help and exit\n"
          "  -i, --build-info         display build information and exit\n"
          "  -l, --license            display WeeChat license and exit\n"
          "  -p, --no-plugin          don't load any plugin at startup\n"
          "  -P, --plugins <plugins>  load only these plugins at startup\n"
          "                           (see /help weechat.plugin.autoload)\n"
          "  -r, --run-command <cmd>  run command(s) after startup;\n"
          "                           many commands can be separated by "
          "semicolons and are evaluated,\n"
          "                           this option can be given multiple "
          "times\n"
          "  -s, --no-script          don't load any script at startup\n"
          "      --upgrade            upgrade WeeChat using session files "
          "(see /help upgrade in WeeChat)\n"
          "  -v, --version            display WeeChat version and exit\n"
          "  plugin:option            option for plugin (see man weechat)\n"));
    string_fprintf (stdout, "\n");

    /* extra options in headless mode */
    if (weechat_headless)
    {
        string_fprintf (stdout, _("Extra options in headless mode:\n"));
        string_fprintf (
            stdout,
            _("      --doc-gen <path>     generate files to build "
              "documentation and exit\n"));
        string_fprintf (
            stdout,
            _("      --daemon             run WeeChat as a daemon (fork, "
              "new process group, file descriptors closed);\n"));
        string_fprintf (
            stdout,
            _("                           (by default in headless mode "
              "WeeChat is blocking and does not run in background)\n"));
        string_fprintf (
            stdout,
            _("      --stdout             display log messages on standard "
              "output instead of writing them in log file\n"));
        string_fprintf (
            stdout,
            _("                           (option ignored if option "
              "\"--daemon\" is given)\n"));
        string_fprintf (stdout, "\n");
    }

    /* debug options */
    string_fprintf (
        stdout,
        _("Debug options (for tools like valgrind, DO NOT USE IN PRODUCTION):\n"
          "      --no-dlclose         do not call function dlclose after "
          "plugins are unloaded\n"
          "      --no-gnutls          disable init/deinit of gnutls\n"
          "      --no-gcrypt          disable init/deinit of gcrypt\n"));
    string_fprintf (stdout, "\n");
}

/*
 * Parses command line arguments.
 *
 * Arguments argc and argv come from main() function.
 */

void
weechat_parse_args (int argc, char *argv[])
{
    int opt;
    struct option long_options[] = {
        /* standard options */
        { "no-connect",  no_argument,       NULL, 'a'               },
        { "colors",      no_argument,       NULL, 'c'               },
        { "dir",         required_argument, NULL, 'd'               },
        { "temp-dir",    no_argument,       NULL, 't'               },
        { "help",        no_argument,       NULL, 'h'               },
        { "build-info",  no_argument,       NULL, 'i'               },
        { "license",     no_argument,       NULL, 'l'               },
        { "no-plugin",   no_argument,       NULL, 'p'               },
        { "plugins",     required_argument, NULL, 'P'               },
        { "run-command", required_argument, NULL, 'r'               },
        { "no-script",   no_argument,       NULL, 's'               },
        { "upgrade",     no_argument,       NULL, 'u'               },
        { "doc-gen",     required_argument, NULL, OPTION_DOCGEN     },
        { "version",     no_argument,       NULL, 'v'               },
        /* debug options */
        { "no-dlclose",  no_argument,       NULL, OPTION_NO_DLCLOSE },
        { "no-gnutls",   no_argument,       NULL, OPTION_NO_GNUTLS  },
        { "no-gcrypt",   no_argument,       NULL, OPTION_NO_GCRYPT  },
        { NULL,          0,                 NULL, 0                 },
    };

    weechat_argv0 = (argv[0]) ? strdup (argv[0]) : NULL;
    weechat_upgrading = 0;
    weechat_home_force = NULL;
    weechat_home_temp = 0;
    weechat_home_delete_on_exit = 0;
    weechat_server_cmd_line = 0;
    weechat_force_plugin_autoload = NULL;
    weechat_doc_gen = 0;
    weechat_plugin_no_dlclose = 0;

    optind = 0;
    opterr = 0;

    while ((opt = getopt_long (argc, argv,
                               ":acd:thilpP:r:sv",
                               long_options, NULL)) != -1)
    {
        switch (opt)
        {
            case 'a': /* -a / --no-connect */
                /* option ignored, it will be used by plugins/scripts */
                break;
            case 'c': /* -c / --colors */
                gui_color_display_terminal_colors ();
                weechat_shutdown (EXIT_SUCCESS, 0);
                break;
            case 'd': /* -d / --dir */
                weechat_home_temp = 0;
                free (weechat_home_force);
                weechat_home_force = strdup (optarg);
                break;
            case 't': /* -t / --temp-dir */
                weechat_home_temp = 1;
                if (weechat_home_force)
                {
                    free (weechat_home_force);
                    weechat_home_force = NULL;
                }
                break;
            case 'h': /* -h / --help */
                weechat_display_usage ();
                weechat_shutdown (EXIT_SUCCESS, 0);
                break;
            case 'i': /* -i / --build-info */
                debug_build_info ();
                weechat_shutdown (EXIT_SUCCESS, 0);
                break;
            case 'l': /* -l / --license */
                weechat_display_copyright ();
                string_fprintf (stdout, "\n");
                string_fprintf (stdout, "%s%s", WEECHAT_LICENSE_TEXT);
                weechat_shutdown (EXIT_SUCCESS, 0);
                break;
            case 'p': /* -p / --no-plugin */
                free (weechat_force_plugin_autoload);
                weechat_force_plugin_autoload = strdup ("!*");
                break;
            case 'P': /* -P / --plugins */
                free (weechat_force_plugin_autoload);
                weechat_force_plugin_autoload = strdup (optarg);
                break;
            case 'r': /* -r / --run-command */
                if (!weechat_startup_commands)
                    weechat_startup_commands = weelist_new ();
                weelist_add (weechat_startup_commands, optarg,
                             WEECHAT_LIST_POS_END, NULL);
                break;
            case 's': /* -s / --no-script */
                /* option ignored, it will be used by the scripting plugins */
                break;
            case 'u': /* --upgrade */
                weechat_upgrading = 1;
                break;
            case OPTION_DOCGEN: /* --doc-gen */
                if (weechat_headless)
                {
                    weechat_doc_gen = 1;
                    weechat_doc_gen_path = strdup (optarg);
                }
                break;
            case 'v': /* -v / --version */
                string_fprintf (stdout, version_get_version ());
                fprintf (stdout, "\n");
                weechat_shutdown (EXIT_SUCCESS, 0);
                break;
            case OPTION_NO_DLCLOSE: /* --no-dlclose */
                /*
                 * Valgrind works better when dlclose() is not done after
                 * plugins are unloaded, it can display stack for plugins,*
                 * otherwise you'll see "???" in stack for functions of
                 * unloaded plugins.
                 * This option disables the call to dlclose(),
                 * it must NOT be used for other purposes!
                 */
                weechat_plugin_no_dlclose = 1;
                break;
            case OPTION_NO_GNUTLS: /* --no-gnutls */
                /*
                 * Electric-fence is not working fine when gnutls loads
                 * certificates and Valgrind reports many memory errors with
                 * gnutls.
                 * This option disables the init/deinit of gnutls,
                 * it must NOT be used for other purposes!
                 */
                weechat_no_gnutls = 1;
                break;
            case OPTION_NO_GCRYPT: /* --no-gcrypt */
                /*
                 * Valgrind reports many memory errors with gcrypt.
                 * This option disables the init/deinit of gcrypt,
                 * it must NOT be used for other purposes!
                 */
                weechat_no_gcrypt = 1;
                break;
            case ':':
                string_fprintf (stderr,
                                _("Error: missing argument for \"%s\" option\n"),
                                argv[optind - 1]);
                weechat_shutdown (EXIT_FAILURE, 0);
                break;
            case '?':
                /* ignore any unknown option; plugins can use them */
                break;
            default:
                /* ignore any other error */
                break;
        }
    }
}

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
    weechat_parse_args (argc, argv);    /* parse command line args          */
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
