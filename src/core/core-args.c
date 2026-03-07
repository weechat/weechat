/*
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

/* Function for command-line arguments */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "weechat.h"
#include "core-debug.h"
#include "core-list.h"
#include "core-string.h"
#include "core-version.h"
#include "../gui/gui-color.h"
#include "../plugins/plugin.h"

/* some command line options */
#define OPTION_DOCGEN     1000
#define OPTION_NO_DLCLOSE 1001
#define OPTION_NO_GNUTLS  1002
#define OPTION_NO_GCRYPT  1003


/*
 * Displays WeeChat copyright on standard output.
 */

void
args_display_copyright (void)
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
 * Displays WeeChat help on standard output.
 */

void
args_display_help (void)
{
    args_display_copyright ();
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
args_parse (int argc, char *argv[])
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
                args_display_help ();
                weechat_shutdown (EXIT_SUCCESS, 0);
                break;
            case 'i': /* -i / --build-info */
                debug_build_info ();
                weechat_shutdown (EXIT_SUCCESS, 0);
                break;
            case 'l': /* -l / --license */
                args_display_copyright ();
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
