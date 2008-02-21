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
 * ##                     http://weechat.flashtux.org                      ##
 * ##                                                                      ##
 * ##########################################################################
 *
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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
#include "wee-backtrace.h"
#include "wee-command.h"
#include "wee-config.h"
#include "wee-debug.h"
#include "wee-hook.h"
#include "wee-log.h"
#include "wee-string.h"
#include "wee-utf8.h"
#include "wee-util.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-main.h"
#include "../gui/gui-keyboard.h"
#include "../plugins/plugin.h"


char *weechat_argv0 = NULL;   /* WeeChat binary file name (argv[0])         */
char *weechat_session = NULL; /* WeeChat session file (for /upgrade cmd)    */
time_t weechat_start_time;    /* WeeChat start time (used by /uptime cmd)   */
int quit_weechat;             /* = 1 if quit request from user... why ? :'( */
int sigsegv = 0;              /* SIGSEGV received?                          */
char *weechat_home = NULL;    /* WeeChat home dir. (default: ~/.weechat)    */

char *local_charset = NULL;   /* local charset, for ex.: ISO-8859-1, UTF-8  */

int server_cmd_line;          /* at least 1 server on WeeChat command line  */
int auto_connect;             /* enabled by default (cmd option to disable) */
int auto_load_plugins;        /* enabled by default (cmd option to disable) */


/*
 * weechat_display_usage: display WeeChat usage
 */

void
weechat_display_usage (char *exec_name)
{
    string_iconv_fprintf (stdout, "\n");
    string_iconv_fprintf (stdout,
                          _("%s (c) Copyright 2003-2008, compiled on %s %s\n"
                            "Developed by FlashCode <flashcode@flashtux.org> "
                            "- %s"),
                          PACKAGE_STRING, __DATE__, __TIME__, WEECHAT_WEBSITE);
    string_iconv_fprintf (stdout, "\n\n");
    string_iconv_fprintf (stdout,
                          _("Usage: %s [options ...]\n"                 \
                            "   or: %s [irc[6][s]://[nickname[:password]@]"
                            "irc.example.org[:port][/channel][,channel[...]]"),
                          exec_name, exec_name);
    string_iconv_fprintf (stdout, "\n\n");
    string_iconv_fprintf (stdout,
                          _("  -a, --no-connect      disable auto-connect to servers at startup\n"
                            "  -c, --config          display config file options\n"
                            "  -d, --dir <path>      set WeeChat home directory (default: ~/.weechat)\n"
                            "  -f, --key-functions   display WeeChat internal functions for keys\n"
                            "  -h, --help            this help\n"
                            "  -m, --commands        display WeeChat commands\n"
                            "  -k, --keys            display WeeChat default keys\n"
                            "  -l, --license         display WeeChat license\n"
                            "  -p, --no-plugin       don't load any plugin at startup\n"
                            "  -v, --version         display WeeChat version\n\n"));
    string_iconv_fprintf(stdout, "\n");
}

/*
 * weechat_display_config_options: display config options
 */

void
weechat_display_config_options ()
{
    string_iconv_fprintf (stdout,
                          /* TRANSLATORS: %s is "WeeChat" */
                          _("%s configuration options:\n"),
                          PACKAGE_NAME);
    config_file_print_stdout (weechat_config_file);
}

/*
 * weechat_display_commands: display commands for one or more protocols
 */

void
weechat_display_commands ()
{
    string_iconv_fprintf (stdout,
                          /* TRANSLATORS: %s is "WeeChat" */
                           _("%s internal commands:\n"),
                          PACKAGE_NAME);
    string_iconv_fprintf (stdout, "\n");
    command_print_stdout ();
}

/*
 * weechat_display_key_functions: display WeeChat key functions
 */

void
weechat_display_key_functions ()
{
    int i;
    
    string_iconv_fprintf (stdout, _("Internal key functions:\n"));
    string_iconv_fprintf (stdout, "\n");
    i = 0;
    while (gui_key_functions[i].function_name)
    {
        string_iconv_fprintf (stdout,
                              "* %s: %s\n",
                              gui_key_functions[i].function_name,
                              _(gui_key_functions[i].description));
        i++;
    }
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
                          /* TRANSLATORS: %s is "WeeChat" */
                          _("%s default keys:\n"),
                          PACKAGE_NAME);
    string_iconv_fprintf (stdout, "\n");
    for (ptr_key = gui_keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        expanded_name = gui_keyboard_get_expanded_name (ptr_key->key);
        string_iconv_fprintf (stdout,
                              "* %s => %s\n",
                              (expanded_name) ? expanded_name : ptr_key->key,
                              (ptr_key->function) ?
                              gui_keyboard_function_search_by_ptr (ptr_key->function) : ptr_key->command);
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
    weechat_session = NULL;
    weechat_home = NULL;
    server_cmd_line = 0;
    auto_connect = 1;
    auto_load_plugins = 1;
    
    for (i = 1; i < argc; i++)
    {
        if ((strcmp (argv[i], "-a") == 0)
            || (strcmp (argv[i], "--no-connect") == 0))
            auto_connect = 0;
        else if ((strcmp (argv[i], "-c") == 0)
            || (strcmp (argv[i], "--config") == 0))
        {
            if (i + 1 < argc)
            {
                weechat_display_config_options (argv[i + 1]);
                weechat_shutdown (EXIT_SUCCESS, 0);
            }
            else
            {
                string_iconv_fprintf (stderr,
                                      _("Error: missing argument for \"%s\" "
                                        "option\n"),
                                      "--config");
                weechat_shutdown (EXIT_FAILURE, 0);
            }
        }
        else if ((strcmp (argv[i], "-d") == 0)
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
        else if ((strcmp (argv[i], "-f") == 0)
            || (strcmp (argv[i], "--key-functions") == 0))
        {
            weechat_display_key_functions ();
            weechat_shutdown (EXIT_SUCCESS, 0);
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
        else if ((strcmp (argv[i], "-m") == 0)
                 || (strcmp (argv[i], "--commands") == 0))
        {
            weechat_display_commands ();
            weechat_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((strcmp (argv[i], "-p") == 0)
                 || (strcmp (argv[i], "--no-plugin") == 0))
        {
            auto_load_plugins = 0;
        }
        else if (strcmp (argv[i], "--session") == 0)
        {
            if (i + 1 < argc)
                weechat_session = strdup (argv[++i]);
            else
            {
                string_iconv_fprintf (stderr,
                                      _("Error: missing argument for \"%s\" "
                                        "option\n"),
                                      "--session");
                weechat_shutdown (EXIT_FAILURE, 0);
            }
        }
        else if ((strcmp (argv[i], "-v") == 0)
                 || (strcmp (argv[i], "--version") == 0))
        {
            string_iconv_fprintf (stdout, PACKAGE_VERSION "\n");
            weechat_shutdown (EXIT_SUCCESS, 0);
        }
        /*else if ((weechat_strncasecmp (argv[i], "irc", 3) == 0))
        {
            if (irc_server_init_with_url (argv[i], &server_tmp) < 0)
            {
                string_iconv_fprintf (stderr,
                                      _("Warning: invalid syntax for IRC server "
                                        "('%s'), ignored\n"),
                                      argv[i]);
            }
            else
            {
                if (!irc_server_new (server_tmp.name, server_tmp.autoconnect,
                                     server_tmp.autoreconnect,
                                     server_tmp.autoreconnect_delay,
                                     1, server_tmp.address, server_tmp.port,
                                     server_tmp.ipv6, server_tmp.ssl,
                                     server_tmp.password, server_tmp.nick1,
                                     server_tmp.nick2, server_tmp.nick3,
                                     NULL, NULL, NULL, NULL, 0,
                                     server_tmp.autojoin, 1, NULL))
                    string_iconv_fprintf (stderr,
                                          _("Warning: unable to create server "
                                            "('%s'), ignored\n"),
                                          argv[i]);
                irc_server_free_data (&server_tmp);
                server_cmd_line = 1;
            }
            }*/
        else
        {
            string_iconv_fprintf (stderr,
                                  _("Warning: unknown parameter '%s', ignored\n"),
                                  argv[i]);
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
        weechat_home =
            (char *)malloc (dir_length * sizeof (char));
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
    if (!util_create_dir (weechat_home, 0))
    {
        string_iconv_fprintf (stderr,
                              _("Error: unable to create \"%s\" directory\n"),
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
    if (CONFIG_BOOLEAN(config_look_startup_logo))
    {
        gui_chat_printf (NULL,
                         "%s   ___       __         ______________        _____ \n"
                         "%s   __ |     / /___________  ____/__  /_______ __  /_\n"
                         "%s   __ | /| / /_  _ \\  _ \\  /    __  __ \\  __ `/  __/\n"
                         "%s   __ |/ |/ / /  __/  __/ /___  _  / / / /_/ // /_  \n"
                         "%s   ____/|__/  \\___/\\___/\\____/  /_/ /_/\\__,_/ \\__/  ",
                         GUI_COLOR(GUI_COLOR_CHAT_NICK),
                         GUI_COLOR(GUI_COLOR_CHAT_NICK),
                         GUI_COLOR(GUI_COLOR_CHAT_NICK),
                         GUI_COLOR(GUI_COLOR_CHAT_NICK),
                         GUI_COLOR(GUI_COLOR_CHAT_NICK));
    }
    if (CONFIG_STRING(config_look_weechat_slogan)
        && CONFIG_STRING(config_look_weechat_slogan)[0])
    {
        gui_chat_printf (NULL, _("%sWelcome to %s%s%s, %s"),
                         (CONFIG_BOOLEAN(config_look_startup_logo)) ?
                         "      " : "",
                         GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                         PACKAGE_NAME,
                         GUI_NO_COLOR,
                         CONFIG_STRING(config_look_weechat_slogan));
    }
    if (CONFIG_BOOLEAN(config_look_startup_version))
    {
        gui_chat_printf (NULL, "%s%s%s%s, %s %s %s",
                         (CONFIG_BOOLEAN(config_look_startup_logo)) ?
                         "    " : "",
                         GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                         PACKAGE_STRING,
                         GUI_NO_COLOR,
                         _("compiled on"), __DATE__, __TIME__);
    }
    if (CONFIG_BOOLEAN(config_look_startup_logo) ||
        (CONFIG_STRING(config_look_weechat_slogan)
         && CONFIG_STRING(config_look_weechat_slogan)[0]) ||
        CONFIG_BOOLEAN(config_look_startup_version))
        gui_chat_printf (NULL,
                         "%s-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-",
                         GUI_COLOR(GUI_COLOR_CHAT_NICK));
    
    log_printf ("%s (%s %s %s)",
                PACKAGE_STRING, _("compiled on"), __DATE__, __TIME__);
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
    if (local_charset)
        free (local_charset);
    
    if (crash)
        abort();
    else
        exit (return_code);
}

/*
 * weechat_sigsegv: SIGSEGV handler: save crash log to
 *                  <weechat_home>/weechat.log and exit
 */

void
weechat_sigsegv ()
{
    debug_dump (1);
    unhook_all ();
    gui_main_end ();

    string_iconv_fprintf (stderr, "\n");
    string_iconv_fprintf (stderr, "*** Very bad! WeeChat is crashing (SIGSEGV received)\n");
    if (!log_crash_rename ())
        string_iconv_fprintf (stderr,
                              "*** Full crash dump was saved to %s/weechat.log file.\n",
                              weechat_home);
    string_iconv_fprintf (stderr, "***\n");
    string_iconv_fprintf (stderr, "*** Please help WeeChat developers to fix this bug:\n");
    string_iconv_fprintf (stderr, "***   1. If you have a core file, please run:  gdb weechat-curses core\n");
    string_iconv_fprintf (stderr, "***      then issue \"bt\" command and send result to developers\n");
    string_iconv_fprintf (stderr, "***      To enable core files with bash shell: ulimit -c 10000\n");
    string_iconv_fprintf (stderr, "***   2. Otherwise send backtrace (below) and weechat.log\n");
    string_iconv_fprintf (stderr, "***      (be careful, private info may be in this file since\n");
    string_iconv_fprintf (stderr, "***      part of chats are displayed, so remove lines if needed)\n\n");
    
    weechat_backtrace ();
    
    /* shutdown with error code */
    weechat_shutdown (EXIT_FAILURE, 1);
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
    local_charset = strdup (nl_langinfo (CODESET));
#else
    local_charset = strdup ("");
#endif
    utf8_init ();
    
    util_catch_signal (SIGINT, SIG_IGN);  /* ignore SIGINT signal           */
    util_catch_signal (SIGQUIT, SIG_IGN); /* ignore SIGQUIT signal          */
    util_catch_signal (SIGPIPE, SIG_IGN); /* ignore SIGPIPE signal          */
    util_catch_signal (SIGSEGV,
                       &weechat_sigsegv); /* crash dump for SIGSEGV signal  */
    hook_init ();                       /* initialize hooks                 */
    debug_init ();                      /* hook signals for debug           */ 
    gui_main_pre_init (&argc, &argv);   /* pre-initiliaze interface         */
    weechat_init_vars ();               /* initialize some variables        */
    command_init ();                    /* initialize WeeChat commands      */
    gui_keyboard_init ();               /* init keyb. (default key bindings)*/
    if (!config_weechat_init ())        /* init options with default values */
        exit (EXIT_FAILURE);
    weechat_parse_args (argc, argv);    /* parse command line args          */
    weechat_create_home_dirs ();        /* create WeeChat directories       */
    log_init ();                        /* init log file                    */
    if (config_weechat_read () < 0)     /* read WeeChat configuration       */
        exit (EXIT_FAILURE);
    gui_main_init ();                   /* init WeeChat interface           */
    //if (weechat_session)
        //session_load (weechat_session); /* load previous session if asked   */
    weechat_welcome_message ();         /* display WeeChat welcome message  */
    plugin_init (auto_load_plugins);    /* init plugin interface(s)         */
    gui_main_loop ();                   /* WeeChat main loop                */
    plugin_end ();                      /* end plugin interface(s)          */
    if (CONFIG_BOOLEAN(config_look_save_on_exit))
        (void) config_weechat_write (NULL); /* save WeeChat config file     */
    gui_main_end ();                    /* shut down WeeChat GUI            */
    unhook_all ();                      /* remove all hooks                 */
    weechat_shutdown (EXIT_SUCCESS, 0); /* quit WeeChat (oh no, why?)       */
    
    return EXIT_SUCCESS;                /* make C compiler happy            */
}
