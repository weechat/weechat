/* ############################################################################
 * ###          ___       __         ______________        _____            ###
 * ###          __ |     / /___________  ____/__  /_______ __  /_           ###
 * ###          __ | /| / /_  _ \  _ \  /    __  __ \  __ `/  __/           ###
 * ###          __ |/ |/ / /  __/  __/ /___  _  / / / /_/ // /_             ###
 * ###          ____/|__/  \___/\___/\____/  /_/ /_/\__,_/ \__/             ###
 * ###                                                                      ###
 * ###             WeeChat - Wee Enhanced Environment for Chat              ###
 * ###                 Fast & light environment for Chat                    ###
 * ###                                                                      ###
 * ###               By: FlashCode <flashcode@flashtux.org>                 ###
 * ###                   Bounga <bounga@altern.org>                         ###
 * ###                   Xahlexx <xahlexx@tuxisland.org>                    ###
 * ###                                                                      ###
 * ###                   http://weechat.flashtux.org                        ###
 * ###                                                                      ###
 * ############################################################################
 *
 * Copyright (c) 2004 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* weechat.c: core functions for WeeChat */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>

#include "weechat.h"
#include "weeconfig.h"
#include "command.h"
#include "../irc/irc.h"
#include "../gui/gui.h"
#include "../plugins/plugins.h"


int quit_weechat;       /* = 1 if quit request from user... why ? :'(        */
char *weechat_home;     /* WeeChat home dir. (example: /home/toto/.weechat)  */
FILE *log_file;         /* WeeChat log file (~/.weechat/weechat.log)         */


/*
 * my_sigint: SIGINT handler, do nothing (just ignore this signal)
 *            Prevents user for exiting with Ctrl-C
 */

void
my_sigint ()
{
    /* do nothing */
}

/*
 * wee_log_printf: displays a message in WeeChat log (~/.weechat/weechat.log)
 */

void
wee_log_printf (char *message, ...)
{
    static char buffer[4096];
    va_list argptr;
    static time_t seconds;
    struct tm *date_tmp;
    
    if (!log_file)
        return;
    
    va_start (argptr, message);
    vsnprintf (buffer, sizeof (buffer) - 1, message, argptr);
    va_end (argptr);
    
    seconds = time (NULL);
    date_tmp = localtime (&seconds);
    fprintf (log_file, "[%04d-%02d-%02d %02d:%02d:%02d] %s",
             date_tmp->tm_year + 1900, date_tmp->tm_mon + 1, date_tmp->tm_mday,
             date_tmp->tm_hour, date_tmp->tm_min, date_tmp->tm_sec,
             buffer);
    fflush (log_file);
}

/*
 * wee_parse_args: parse command line args
 */

void
wee_parse_args (int argc, char *argv[])
{
    int i, j, k, m;

    for (i = 1; i < argc; i++)
    {
        if ((strcmp (argv[i], "-c") == 0)
            || (strcmp (argv[i], "--config") == 0))
        {
            printf(_("WeeChat configuration options (~/.weechat/weechat.rc):\n\n"));
            for (j = 0; j < CONFIG_NUMBER_SECTIONS; j++)
            {
                if (weechat_options[j])
                {
                    k = 0;
                    while (weechat_options[j][k].option_name)
                    {
                        printf ("* %s:\n",
                            weechat_options[j][k].option_name);
                        switch (weechat_options[j][k].option_type)
                        {
                            case OPTION_TYPE_BOOLEAN:
                                printf (_("  . type boolean (values: 'on' or 'off')\n"));
                                printf (_("  . default value: '%s'\n"),
                                    (weechat_options[j][k].default_int == BOOL_TRUE) ?
                                    "on" : "off");
                                break;
                            case OPTION_TYPE_INT:
                                printf (_("  . type integer (values: between %d and %d)\n"),
                                    weechat_options[j][k].min,
                                    weechat_options[j][k].max);
                                printf (_("  . default value: %d\n"),
                                    weechat_options[j][k].default_int);
                                break;
                            case OPTION_TYPE_INT_WITH_STRING:
                                printf (_("  . type string (values: "));
                                m = 0;
                                while (weechat_options[j][k].array_values[m])
                                {
                                    printf ("'%s'",
                                        weechat_options[j][k].array_values[m]);
                                    if (weechat_options[j][k].array_values[m + 1])
                                        printf (", ");
                                    m++;
                                }
                                printf (")\n");
                                printf (_("  . default value: '%s'\n"),
                                    (weechat_options[j][k].default_string) ?
                                    weechat_options[j][k].default_string : _("empty"));
                                break;
                            case OPTION_TYPE_COLOR:
                                printf (_("  . type color (Curses or Gtk color, look at WeeChat doc)\n"));
                                printf (_("  . default value: '%s'\n"),
                                    (weechat_options[j][k].default_string) ?
                                    weechat_options[j][k].default_string : _("empty"));
                                break;
                            case OPTION_TYPE_STRING:
                                printf (_("  . type string (any string)\n"));
                                printf (_("  . default value: '%s'\n"),
                                    (weechat_options[j][k].default_string) ?
                                    weechat_options[j][k].default_string : _("empty"));
                                break;
                        }
                        printf (_("  . description: %s\n\n"),
                            gettext (weechat_options[j][k].long_description));
                        k++;
                    }
                }
            }
            printf (_("Moreover, you can define aliases in [alias] section, by adding lines like:\n"));
            printf ("j=join\n");
            printf (_("where 'j' is alias name, and 'join' associated command.\n\n"));
            exit (0);
        }
        else if ((strcmp (argv[i], "-h") == 0)
                || (strcmp (argv[i], "--help") == 0))
        {
            printf ("\n%s%s", WEE_USAGE);
            exit (0);
        }
        else if ((strcmp (argv[i], "-l") == 0)
                 || (strcmp (argv[i], "--license") == 0))
        {
            printf ("\n%s%s", WEE_LICENSE);
            exit (0);
        }
        else if ((strcmp (argv[i], "-v") == 0)
                 || (strcmp (argv[i], "--version") == 0))
        {
            printf (PACKAGE_VERSION "\n");
            exit (0);
        }
        else
        {
            fprintf (stderr,
                     _("%s unknown parameter '%s', ignored\n"),
                     WEECHAT_WARNING, argv[i]);
        }
    }
}

/*
 * wee_create_dir: create a directory
 *                 return: 1 if ok (or directory already exists)
 *                         0 if error
 */

int
wee_create_dir (char *directory)
{
    if (mkdir (directory, 0755) < 0)
    {
        /* exit if error (except if directory already exists) */
        if (errno != EEXIST)
        {
            fprintf (stderr, _("%s cannot create directory \"%s\"\n"),
                     WEECHAT_ERROR, directory);
            return 0;
        }
    }
    return 1;
}

/*
 * wee_create_home_dirs: create (if not found):
 *                       - WeeChat home directory ("~/.weechat")
 *                       - "perl" directory (and "autoload")
 *                       - "ruby" directory (and "autoload")
 *                       - "python" directory (and "autoload")
 */

void
wee_create_home_dirs ()
{
    char *dir_name;

    /* TODO: rewrite this code for Windows version */
    weechat_home =
        (char *) malloc ((strlen (getenv ("HOME")) + 10) * sizeof (char));
    sprintf (weechat_home, "%s%s.weechat", getenv ("HOME"), DIR_SEPARATOR);
    
    /* create home directory "~/.weechat" ; error is fatal */
    if (!wee_create_dir (weechat_home))
        exit (1);
    
    dir_name = (char *) malloc ((strlen (weechat_home) + 64) * sizeof (char));
    
    #ifdef PLUGIN_PERL
    /* create "~/.weechat/perl" */
    sprintf (dir_name, "%s%s%s", weechat_home, DIR_SEPARATOR, "perl");
    if (wee_create_dir (dir_name))
    {
        /* create "~/.weechat/perl/autoload" */
        sprintf (dir_name, "%s%s%s%s%s", weechat_home, DIR_SEPARATOR, "perl",
                 DIR_SEPARATOR, "autoload");
        wee_create_dir (dir_name);
    }
    #endif
    
    #ifdef PLUGIN_PYTHON
    /* create "~/.weechat/python" */
    sprintf (dir_name, "%s%s%s", weechat_home, DIR_SEPARATOR, "python");
    if (wee_create_dir (dir_name))
    {
        /* create "~/.weechat/python/autoload" */
        sprintf (dir_name, "%s%s%s%s%s", weechat_home, DIR_SEPARATOR, "python",
                 DIR_SEPARATOR, "autoload");
        wee_create_dir (dir_name);
    }
    #endif
    
    #ifdef PLUGIN_RUBY
    /* create "~/.weechat/ruby" */
    sprintf (dir_name, "%s%s%s", weechat_home, DIR_SEPARATOR, "ruby");
    if (wee_create_dir (dir_name))
    {
        /* create "~/.weechat/ruby/autoload" */
        sprintf (dir_name, "%s%s%s%s%s", weechat_home, DIR_SEPARATOR, "ruby",
                 DIR_SEPARATOR, "autoload");
        wee_create_dir (dir_name);
    }
    #endif
    
    free (dir_name);
}

/*
 * wee_init_vars: initialize some variables
 */

void
wee_init_vars ()
{
    /* GUI not yet initialized */
    gui_ready = 0;

    /* init received messages queue */
    recv_msgq = NULL;
    msgq_last_msg = NULL;
}

/*
 * wee_init_log: initialize log file
 */

void
wee_init_log ()
{
    char *filename;
    
    filename =
        (char *) malloc ((strlen (weechat_home) + 64) * sizeof (char));
    sprintf (filename, "%s/" WEECHAT_LOG_NAME, weechat_home);
    if ((log_file = fopen (filename, "wt")) == NULL)
    {
        free (filename);
        fprintf (stderr,
                 _("%s unable to create/append to log file (~/.weechat/%s)"),
                 WEECHAT_ERROR, WEECHAT_LOG_NAME);
    }
    free (filename);
}

/*
 * weechat_welcome_message: display WeeChat welcome message - yeah!
 */

void
weechat_welcome_message ()
{
    if (cfg_look_startup_logo)
    {
        gui_printf_color (NULL, COLOR_WIN_CHAT_PREFIX1,
            "   ___       __         ______________        _____ \n"
            "   __ |     / /___________  ____/__  /_______ __  /_\n"
            "   __ | /| / /_  _ \\  _ \\  /    __  __ \\  __ `/  __/\n"
            "   __ |/ |/ / /  __/  __/ /___  _  / / / /_/ // /_  \n"
            "   ____/|__/  \\___/\\___/\\____/  /_/ /_/\\__,_/ \\__/  \n");
    }
    if (cfg_look_weechat_slogan && cfg_look_weechat_slogan[0])
    {
        gui_printf_color (NULL, COLOR_WIN_CHAT, _("%sWelcome to "),
                          (cfg_look_startup_logo) ? "      " : "");
        gui_printf_color (NULL, COLOR_WIN_CHAT_PREFIX2, PACKAGE_NAME);
        gui_printf_color (NULL, COLOR_WIN_CHAT,
                          ", %s\n", cfg_look_weechat_slogan);
    }
    if (cfg_look_startup_version)
    {
        gui_printf_color (NULL, COLOR_WIN_CHAT_PREFIX2,
                          "%s" PACKAGE_STRING,
                          (cfg_look_startup_logo) ? "    " : "");
        gui_printf_color (NULL, COLOR_WIN_CHAT,
                          ", %s %s %s\n",
                          _("compiled on"), __DATE__, __TIME__);
    }
    if (cfg_look_startup_logo ||
        (cfg_look_weechat_slogan && cfg_look_weechat_slogan[0]) ||
        cfg_look_startup_version)
        gui_printf_color (NULL, COLOR_WIN_CHAT_PREFIX1,
            "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
}

/*
 * wee_shutdown: shutdown WeeChat
 */

void
wee_shutdown ()
{
    server_free_all ();
    gui_end ();
    if (log_file)
        fclose (log_file);
    exit (0);
}

/*
 * main: WeeChat startup
 */

int
main (int argc, char *argv[])
{
    #ifdef ENABLE_NLS
    setlocale (LC_ALL, "");         /* initialize gettext                   */
    bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);
    #endif
    
    signal (SIGINT, my_sigint);     /* ignore SIGINT signal                 */
    gui_pre_init (&argc, &argv);    /* pre-initiliaze interface             */
    wee_init_vars ();               /* initialize some variables            */
    wee_parse_args (argc, argv);    /* parse command line args              */
    wee_create_home_dirs ();        /* create WeeChat directories           */
    wee_init_log ();                /* init log file                        */
    index_command_build ();         /* build commands index  for completion */
    
    switch (config_read ())         /* read configuration                   */
    {
        case 0:                     /* config file OK                       */
            break;
        case -1:                    /* config file not found                */
            if (config_create_default () < 0)
                return 1;
            config_read ();
            break;
        default:                    /* other error (fatal)                  */
            server_free_all ();
            return 1;
    }
    
    gui_init ();                    /* init WeeChat interface               */
    plugin_init ();                 /* init plugin interface(s)             */    
    weechat_welcome_message ();     /* display WeeChat welcome message      */
    server_auto_connect ();         /* auto-connect to servers              */
    
    gui_main_loop ();               /* WeeChat main loop                    */
    
    plugin_end ();                  /* end plugin interface(s)              */
    server_disconnect_all ();       /* disconnect from all servers          */
    config_write (NULL);            /* save config file                     */
    wee_shutdown ();                /* quit WeeChat (oh no, why?)           */
    
    return 0;                       /* make gcc happy (never executed)      */
}
