/* ##########################################################################
 * ##          ___       __         ______________        _____            ##
 * ##          __ |     / /___________  ____/__  /_______ __  /_           ##
 * ##          __ | /| / /_  _ \  _ \  /    __  __ \  __ `/  __/           ##
 * ##          __ |/ |/ / /  __/  __/ /___  _  / / / /_/ // /_             ##
 * ##          ____/|__/  \___/\___/\____/  /_/ /_/\__,_/ \__/             ##
 * ##                                                                      ##
 * ##             WeeChat - Wee Enhanced Environment for Chat              ##
 * ##                 Fast, light, extensible chat client                  ##
 * ##                                                                      ##
 * ##                By FlashCode <flashcode@flashtux.org>                 ##
 * ##                                                                      ##
 * ##                    http://weechat.flashtux.org/                      ##
 * ##                                                                      ##
 * ##########################################################################
 *
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* weechat.c: core functions for WeeChat */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>

#ifdef HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif

#include "weechat.h"
#include "wee-command.h"
#include "wee-config.h"
#include "wee-debug.h"
#include "wee-hook.h"
#include "wee-log.h"
#include "wee-network.h"
#include "wee-proxy.h"
#include "wee-string.h"
#include "wee-upgrade.h"
#include "wee-utf8.h"
#include "wee-util.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-completion.h"
#include "../gui/gui-layout.h"
#include "../gui/gui-main.h"
#include "../gui/gui-keyboard.h"
#include "../plugins/plugin.h"


int weechat_debug_core = 0;            /* debug level for core              */
char *weechat_argv0 = NULL;            /* WeeChat binary file name (argv[0])*/
int weechat_upgrading = 0;             /* =1 if WeeChat is upgrading        */
time_t weechat_start_time = 0;         /* start time (used by /uptime cmd)  */
int weechat_upgrade_count = 0;         /* number of /upgrade done           */
int weechat_quit = 0;                  /* = 1 if quit request from user     */
int weechat_sigsegv = 0;               /* SIGSEGV received?                 */
char *weechat_home = NULL;             /* home dir. (default: ~/.weechat)   */
char *weechat_local_charset = NULL;    /* example: ISO-8859-1, UTF-8        */
int weechat_server_cmd_line = 0;       /* at least 1 server on cmd line     */
int weechat_auto_load_plugins = 1;     /* auto load plugins                 */
int weechat_plugin_no_dlclose = 0;     /* remove calls to dlclose for libs  */
                                       /* (useful when using valgrind)      */


/*
 * weechat_display_usage: display WeeChat usage
 */

void
weechat_display_usage (char *exec_name)
{
    string_iconv_fprintf (stdout, "\n");
    string_iconv_fprintf (stdout,
                          _("%s (c) Copyright 2003-2009, compiled on %s %s\n"
                            "Developed by FlashCode <flashcode@flashtux.org> "
                            "- %s"),
                          PACKAGE_STRING, __DATE__, __TIME__, WEECHAT_WEBSITE);
    string_iconv_fprintf (stdout, "\n\n");
    string_iconv_fprintf (stdout,
                          _("Usage: %s [option...] [plugin:option...]\n"),
                          exec_name, exec_name);
    string_iconv_fprintf (stdout, "\n\n");
    string_iconv_fprintf (stdout,
                          _("  -a, --no-connect  disable auto-connect to servers at startup\n"
                            "  -d, --dir <path>  set WeeChat home directory (default: ~/.weechat)\n"
                            "  -h, --help        this help\n"
                            "  -k, --keys        display WeeChat default keys\n"
                            "  -l, --license     display WeeChat license\n"
                            "  -p, --no-plugin   don't load any plugin at startup\n"
                            "  -v, --version     display WeeChat version\n"
                            "  plugin:option     option for plugin\n"
                            "                    for example, irc plugin can connect\n"
                            "                    to server with url like:\n"
                            "                    irc[6][s]://[nickname[:password]@]"
                            "irc.example.org[/port][//#channel1][,#channel2[...]]\n"
                            "                    (look at plugins documentation for more information\n"
                            "                    about possible options)\n"));
    string_iconv_fprintf(stdout, "\n");
}

/*
 * weechat_display_keys: display WeeChat default keys
 */

void
weechat_display_keys ()
{
    struct t_gui_key *ptr_key;
    char *expanded_name;
    
    string_iconv_fprintf (stdout,
                          /* TRANSLATORS: %s is "weechat" */
                          _("%s default keys:\n"),
                          PACKAGE_NAME);
    string_iconv_fprintf (stdout, "\n");
    for (ptr_key = gui_keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        expanded_name = gui_keyboard_get_expanded_name (ptr_key->key);
        string_iconv_fprintf (stdout,
                              "* %s => %s\n",
                              (expanded_name) ? expanded_name : ptr_key->key,
                              ptr_key->command);
        if (expanded_name)
            free (expanded_name);
    }
}

/*
 * weechat_parse_args: parse command line args
 */

void
weechat_parse_args (int argc, char *argv[])
{
    int i;
    
    weechat_argv0 = strdup (argv[0]);
    weechat_upgrading = 0;
    weechat_home = NULL;
    weechat_server_cmd_line = 0;
    weechat_auto_load_plugins = 1;
    weechat_plugin_no_dlclose = 0;
    
    for (i = 1; i < argc; i++)
    {
        if ((strcmp (argv[i], "-d") == 0)
            || (strcmp (argv[i], "--dir") == 0))
        {
            if (i + 1 < argc)
                weechat_home = strdup (argv[++i]);
            else
            {
                string_iconv_fprintf (stderr,
                                      _("Error: missing argument for \"%s\" "
                                        "option\n"),
                                      "--dir");
                weechat_shutdown (EXIT_FAILURE, 0);
            }
        }
        else if ((strcmp (argv[i], "-h") == 0)
                || (strcmp (argv[i], "--help") == 0))
        {
            weechat_display_usage (argv[0]);
            weechat_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((strcmp (argv[i], "-k") == 0)
            || (strcmp (argv[i], "--keys") == 0))
        {
            weechat_display_keys ();
            weechat_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((strcmp (argv[i], "-l") == 0)
                 || (strcmp (argv[i], "--license") == 0))
        {
            string_iconv_fprintf (stdout, "\n%s%s", WEECHAT_LICENSE);
            weechat_shutdown (EXIT_SUCCESS, 0);
        }
        else if (strcmp (argv[i], "--no-dlclose") == 0)
        {
            /* tools like valgrind work better when dlclose() is not done
               after plugins are unloaded, they can display stack for plugins,
               otherwise you'll see "???" in stack for functions of unloaded
               plugins -- this option should not be used for other purposes! */
            weechat_plugin_no_dlclose = 1;
        }
        else if ((strcmp (argv[i], "-p") == 0)
                 || (strcmp (argv[i], "--no-plugin") == 0))
        {
            weechat_auto_load_plugins = 0;
        }
        else if (strcmp (argv[i], "--upgrade") == 0)
        {
            weechat_upgrading = 1;
        }
        else if ((strcmp (argv[i], "-v") == 0)
                 || (strcmp (argv[i], "--version") == 0))
        {
            string_iconv_fprintf (stdout, PACKAGE_VERSION "\n");
            weechat_shutdown (EXIT_SUCCESS, 0);
        }
    }
}

/*
 * weechat_create_home_dirs: create WeeChat directories
 */

void
weechat_create_home_dirs ()
{
    char *ptr_home;
    int dir_length;
    struct stat statinfo;

    if (!weechat_home)
    {
        ptr_home = getenv ("HOME");
        if (!ptr_home)
        {
            string_iconv_fprintf (stderr,
                                  _("Error: unable to get HOME directory\n"));
            weechat_shutdown (EXIT_FAILURE, 0);
        }
        dir_length = strlen (ptr_home) + 10;
        weechat_home = malloc (dir_length);
        if (!weechat_home)
        {
            string_iconv_fprintf (stderr,
                                  _("Error: not enough memory for home "
                                    "directory\n"));
            weechat_shutdown (EXIT_FAILURE, 0);
        }
        snprintf (weechat_home, dir_length, "%s%s.weechat", ptr_home,
                  DIR_SEPARATOR);
    }

    /* if home already exists, it has to be a directory */
    if (stat (weechat_home, &statinfo) == 0)
    {
        if (!S_ISDIR (statinfo.st_mode))
        {
            string_iconv_fprintf (stderr,
                                  _("Error: home (%s) is not a directory\n"),
                                  weechat_home);
            weechat_shutdown (EXIT_FAILURE, 0);
        }
    }
    
    /* create home directory; error is fatal */
    if (!util_mkdir (weechat_home, 0755))
    {
        string_iconv_fprintf (stderr,
                              _("Error: cannot create directory \"%s\"\n"),
                              weechat_home);
        weechat_shutdown (EXIT_FAILURE, 0);
    }
}

/*
 * weechat_init_vars: initialize some variables
 */

void
weechat_init_vars ()
{
    /* start time, used by /uptime command */
    weechat_start_time = time (NULL);
}

/*
 * weechat_welcome_message: display WeeChat welcome message - yeah!
 */

void
weechat_welcome_message ()
{
    if (CONFIG_BOOLEAN(config_startup_display_logo))
    {
        gui_chat_printf (NULL,
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
    if (CONFIG_STRING(config_startup_weechat_slogan)
        && CONFIG_STRING(config_startup_weechat_slogan)[0])
    {
        gui_chat_printf (NULL, _("Welcome to %sWeeChat%s, %s"),
                         GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                         GUI_NO_COLOR,
                         CONFIG_STRING(config_startup_weechat_slogan));
    }
    if (CONFIG_BOOLEAN(config_startup_display_version))
    {
        command_version_display (NULL, 0);
    }
    if (CONFIG_BOOLEAN(config_startup_display_logo) ||
        (CONFIG_STRING(config_startup_weechat_slogan)
         && CONFIG_STRING(config_startup_weechat_slogan)[0]) ||
        CONFIG_BOOLEAN(config_startup_display_version))
    {
        gui_chat_printf (NULL,
                         "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    }
}

/*
 * weechat_shutdown: shutdown WeeChat
 */

void
weechat_shutdown (int return_code, int crash)
{
    if (weechat_argv0)
        free (weechat_argv0);
    if (weechat_home)
        free (weechat_home);
    log_close ();
    if (weechat_local_charset)
        free (weechat_local_charset);

    network_end ();
    
    if (crash)
        abort();
    else
        exit (return_code);
}

/*
 * main: WeeChat startup
 */

int
main (int argc, char *argv[])
{
    setlocale (LC_ALL, "");             /* initialize gettext               */
#ifdef ENABLE_NLS
    bindtextdomain (PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (PACKAGE, "UTF-8");
    textdomain (PACKAGE);
#endif
    
#ifdef HAVE_LANGINFO_CODESET
    weechat_local_charset = strdup (nl_langinfo (CODESET));
#else
    weechat_local_charset = strdup ("");
#endif
    utf8_init ();
    network_init ();
    
    util_catch_signal (SIGINT, SIG_IGN);  /* ignore SIGINT signal           */
    util_catch_signal (SIGQUIT, SIG_IGN); /* ignore SIGQUIT signal          */
    util_catch_signal (SIGPIPE, SIG_IGN); /* ignore SIGPIPE signal          */
    util_catch_signal (SIGSEGV,
                       &debug_sigsegv); /* crash dump for SIGSEGV signal    */
    hook_init ();                       /* initialize hooks                 */
    debug_init ();                      /* hook signals for debug           */ 
    gui_main_pre_init (&argc, &argv);   /* pre-initiliaze interface         */
    weechat_init_vars ();               /* initialize some variables        */
    command_init ();                    /* initialize WeeChat commands      */
    gui_completion_init ();             /* add core completion hooks        */
    gui_keyboard_init ();               /* init keyboard                    */
    if (!config_weechat_init ())        /* init options with default values */
        exit (EXIT_FAILURE);
    weechat_parse_args (argc, argv);    /* parse command line args          */
    weechat_create_home_dirs ();        /* create WeeChat directories       */
    log_init ();                        /* init log file                    */
    if (config_weechat_read () < 0)     /* read WeeChat configuration       */
        exit (EXIT_FAILURE);
    gui_main_init ();                   /* init WeeChat interface           */
    if (weechat_upgrading)
    {
        upgrade_weechat_load ();        /* upgrade with session file        */
        weechat_upgrade_count++;        /* increase /upgrade count          */
    }
    weechat_welcome_message ();         /* display WeeChat welcome message  */
    command_startup (0);                /* command executed before plugins  */
    plugin_init (weechat_auto_load_plugins, /* init plugin interface(s)     */
                 argc, argv); 
    command_startup (1);                /* command executed after plugins   */
    gui_layout_window_apply (gui_layout_windows, -1); /* apply saved layout */
    if (weechat_upgrading)
        upgrade_weechat_remove_files ();/* remove .upgrade files            */
    weechat_upgrading = 0;
    
    gui_main_loop ();                   /* WeeChat main loop                */
    
    gui_layout_save_on_exit ();         /* save layout                      */
    plugin_end ();                      /* end plugin interface(s)          */
    if (CONFIG_BOOLEAN(config_look_save_config_on_exit))
        (void) config_weechat_write (NULL); /* save WeeChat config file     */
    gui_main_end (1);                   /* shut down WeeChat GUI            */
    proxy_free_all ();                  /* free all proxies                 */
    config_file_free_all ();            /* free all configuration files     */
    gui_keyboard_end ();                /* end keyboard                     */
    unhook_all ();                      /* remove all hooks                 */
    weechat_shutdown (EXIT_SUCCESS, 0); /* quit WeeChat (oh no, why?)       */
    
    return EXIT_SUCCESS;                /* make C compiler happy            */
}
