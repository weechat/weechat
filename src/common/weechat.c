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
 * ###                   Xahlexx <xahlexx@weeland.org>                      ###
 * ###                                                                      ###
 * ###                   http://weechat.flashtux.org                        ###
 * ###                                                                      ###
 * ############################################################################
 *
 * Copyright (c) 2003-2005 by FlashCode <flashcode@flashtux.org>
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
#include <iconv.h>
#include <langinfo.h>

#include "weechat.h"
#include "weeconfig.h"
#include "command.h"
#include "../irc/irc.h"
#include "../gui/gui.h"
#include "../plugins/plugins.h"


int quit_weechat;       /* = 1 if quit request from user... why ? :'(        */
char *weechat_home;     /* WeeChat home dir. (example: /home/toto/.weechat)  */
FILE *weechat_log_file; /* WeeChat log file (~/.weechat/weechat.log)         */

char *local_charset = NULL;     /* local charset, for example: ISO-8859-1    */

int server_cmd_line;    /* at least one server on WeeChat command line       */


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
 * weechat_convert_encoding: convert string to another encoding
 */

char *
weechat_convert_encoding (char *from_code, char *to_code, char *string)
{
    iconv_t cd;
    char *inbuf, *ptr_inbuf, *outbuf, *ptr_outbuf;
    int inbytesleft, outbytesleft;
    
    if (from_code && from_code[0] && to_code && to_code[0]
        && (strcasecmp(from_code, to_code) != 0))
    {
        cd = iconv_open (to_code, from_code);
        if (cd == (iconv_t)(-1))
            outbuf = strdup (string);
        else
        {
            inbuf = strdup (string);
            ptr_inbuf = inbuf;
            inbytesleft = strlen (inbuf);
            outbytesleft = inbytesleft * 4;
            outbuf = (char *) malloc (outbytesleft + 2);
            ptr_outbuf = outbuf;
            iconv (cd, &ptr_inbuf, &inbytesleft, &ptr_outbuf, &outbytesleft);
            if (inbytesleft != 0)
            {
                free (outbuf);
                outbuf = strdup (string);
            }
            else
                ptr_outbuf[0] = '\0';
            free (inbuf);
            iconv_close (cd);
        }
    }
    else
        outbuf = strdup (string);
    
    return outbuf;
}

/*
 * get_timeval_diff: calculates difference between two times (return in milliseconds)
 */

long get_timeval_diff(struct timeval *tv1, struct timeval *tv2)
{
    long diff_sec, diff_usec;
    
    diff_sec = tv2->tv_sec - tv1->tv_sec;
    diff_usec = tv2->tv_usec - tv1->tv_usec;
    
    if (diff_usec < 0)
    {
        diff_usec += 1000000;
        diff_sec--;
    }
    return ((diff_usec / 1000) + (diff_sec * 1000));
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
    
    if (!weechat_log_file)
        return;
    
    va_start (argptr, message);
    vsnprintf (buffer, sizeof (buffer) - 1, message, argptr);
    va_end (argptr);
    
    seconds = time (NULL);
    date_tmp = localtime (&seconds);
    if (date_tmp)
        fprintf (weechat_log_file, "[%04d-%02d-%02d %02d:%02d:%02d] %s",
                 date_tmp->tm_year + 1900, date_tmp->tm_mon + 1, date_tmp->tm_mday,
                 date_tmp->tm_hour, date_tmp->tm_min, date_tmp->tm_sec,
                 buffer);
    else
        fprintf (weechat_log_file, "%s", buffer);
    fflush (weechat_log_file);
}

/*
 * wee_display_config_options: display config options
 */

void wee_display_config_options ()
{
    int i, j, k;
    
    printf (_("WeeChat configuration options (~/.weechat/weechat.rc):\n\n"));
    for (i = 0; i < CONFIG_NUMBER_SECTIONS; i++)
    {
        if (weechat_options[i])
        {
            j = 0;
            while (weechat_options[i][j].option_name)
            {
                printf ("* %s:\n",
                    weechat_options[i][j].option_name);
                switch (weechat_options[i][j].option_type)
                {
                    case OPTION_TYPE_BOOLEAN:
                        printf (_("  . type boolean (values: 'on' or 'off')\n"));
                        printf (_("  . default value: '%s'\n"),
                            (weechat_options[i][j].default_int == BOOL_TRUE) ?
                            "on" : "off");
                        break;
                    case OPTION_TYPE_INT:
                        printf (_("  . type integer (values: between %d and %d)\n"),
                            weechat_options[i][j].min,
                            weechat_options[i][j].max);
                        printf (_("  . default value: %d\n"),
                            weechat_options[i][j].default_int);
                        break;
                    case OPTION_TYPE_INT_WITH_STRING:
                        printf (_("  . type string (values: "));
                        k = 0;
                        while (weechat_options[i][j].array_values[k])
                        {
                            printf ("'%s'",
                                weechat_options[i][j].array_values[k]);
                            if (weechat_options[i][j].array_values[k + 1])
                                printf (", ");
                            k++;
                        }
                        printf (")\n");
                        printf (_("  . default value: '%s'\n"),
                            (weechat_options[i][j].default_string) ?
                            weechat_options[i][j].default_string : _("empty"));
                        break;
                    case OPTION_TYPE_COLOR:
                        printf (_("  . type color (Curses or Gtk color, look at WeeChat doc)\n"));
                        printf (_("  . default value: '%s'\n"),
                            (weechat_options[i][j].default_string) ?
                            weechat_options[i][j].default_string : _("empty"));
                        break;
                    case OPTION_TYPE_STRING:
                        printf (_("  . type string (any string)\n"));
                        printf (_("  . default value: '%s'\n"),
                            (weechat_options[i][j].default_string) ?
                            weechat_options[i][j].default_string : _("empty"));
                        break;
                }
                printf (_("  . description: %s\n\n"),
                    gettext (weechat_options[i][j].long_description));
                j++;
            }
        }
    }
    printf (_("Moreover, you can define aliases in [alias] section, by adding lines like:\n"));
    printf ("j=join\n");
    printf (_("where 'j' is alias name, and 'join' associated command.\n\n"));
}

/*
 * wee_parse_args: parse command line args
 */

void
wee_parse_args (int argc, char *argv[])
{
    int i;
    t_irc_server server_tmp;

    server_cmd_line = 0;
    
    for (i = 1; i < argc; i++)
    {
        if ((strcmp (argv[i], "-c") == 0)
            || (strcmp (argv[i], "--config") == 0))
        {
            wee_display_config_options ();
            exit (EXIT_SUCCESS);
        }
        else if ((strcmp (argv[i], "-h") == 0)
                || (strcmp (argv[i], "--help") == 0))
        {
            printf ("\n" WEE_USAGE1, argv[0], argv[0]);
            printf ("%s", WEE_USAGE2);
            exit (EXIT_SUCCESS);
        }
        else if ((strcmp (argv[i], "-l") == 0)
                 || (strcmp (argv[i], "--license") == 0))
        {
            printf ("\n%s%s", WEE_LICENSE);
            exit (EXIT_SUCCESS);
        }
        else if ((strcmp (argv[i], "-v") == 0)
                 || (strcmp (argv[i], "--version") == 0))
        {
            printf (PACKAGE_VERSION "\n");
            exit (EXIT_SUCCESS);
        }
        else if ((strncasecmp (argv[i], "irc://", 6) == 0))
        {
            if (server_init_with_url (argv[i], &server_tmp) < 0)
            {
                fprintf (stderr, _("%s invalid syntax for IRC server ('%s'), ignored\n"),
                         WEECHAT_WARNING, argv[i]);
            }
            else
            {
                if (!server_new (server_tmp.name, server_tmp.autoconnect,
                                 server_tmp.autoreconnect,
                                 server_tmp.autoreconnect_delay,
                                 1, server_tmp.address, server_tmp.port,
                                 server_tmp.password, server_tmp.nick1,
                                 server_tmp.nick2, server_tmp.nick3,
                                 NULL, NULL, NULL, 0, server_tmp.autojoin, 1))
                    fprintf (stderr, _("%s unable to create server ('%s'), ignored\n"),
                             WEECHAT_WARNING, argv[i]);
                server_destroy (&server_tmp);
                server_cmd_line = 1;
            }
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
    char *ptr_home, *dir_name;
    int dir_length;

    /* TODO: rewrite this code for Windows version */
    ptr_home = getenv ("HOME");
    if (!ptr_home)
    {
        fprintf (stderr, _("%s unable to get HOME directory\n"),
                 WEECHAT_ERROR);
        exit (EXIT_FAILURE);
    }
    dir_length = strlen (ptr_home) + 10;
    weechat_home =
        (char *) malloc (dir_length * sizeof (char));
    if (!weechat_home)
    {
        fprintf (stderr, _("%s not enough memory for home directory\n"),
                 WEECHAT_ERROR);
        exit (EXIT_FAILURE);
    }
    snprintf (weechat_home, dir_length, "%s%s.weechat", ptr_home,
              DIR_SEPARATOR);
    
    /* create home directory "~/.weechat" ; error is fatal */
    if (!wee_create_dir (weechat_home))
    {
        fprintf (stderr, _("%s unable to create ~/.weechat directory\n"),
                 WEECHAT_ERROR);
        exit (EXIT_FAILURE);
    }
    
    dir_length = strlen (weechat_home) + 64;
    dir_name = (char *) malloc (dir_length * sizeof (char));
    
    #ifdef PLUGIN_PERL
    /* create "~/.weechat/perl" */
    snprintf (dir_name, dir_length, "%s%s%s", weechat_home, DIR_SEPARATOR,
              "perl");
    if (wee_create_dir (dir_name))
    {
        /* create "~/.weechat/perl/autoload" */
        snprintf (dir_name, dir_length, "%s%s%s%s%s", weechat_home,
                 DIR_SEPARATOR, "perl", DIR_SEPARATOR, "autoload");
        wee_create_dir (dir_name);
    }
    #endif
    
    #ifdef PLUGIN_PYTHON
    /* create "~/.weechat/python" */
    snprintf (dir_name, dir_length, "%s%s%s", weechat_home, DIR_SEPARATOR,
             "python");
    if (wee_create_dir (dir_name))
    {
        /* create "~/.weechat/python/autoload" */
        snprintf (dir_name, dir_length, "%s%s%s%s%s", weechat_home,
                 DIR_SEPARATOR, "python", DIR_SEPARATOR, "autoload");
        wee_create_dir (dir_name);
    }
    #endif
    
    #ifdef PLUGIN_RUBY
    /* create "~/.weechat/ruby" */
    snprintf (dir_name, dir_length, "%s%s%s", weechat_home, DIR_SEPARATOR,
             "ruby");
    if (wee_create_dir (dir_name))
    {
        /* create "~/.weechat/ruby/autoload" */
        snprintf (dir_name, dir_length, "%s%s%s%s%s", weechat_home,
                 DIR_SEPARATOR, "ruby", DIR_SEPARATOR, "autoload");
        wee_create_dir (dir_name);
    }
    #endif
    
    /* create "~/.weechat/logs" */
    snprintf (dir_name, dir_length, "%s%s%s", weechat_home, DIR_SEPARATOR,
              "logs");
    if (!wee_create_dir (dir_name))
    {
        fprintf (stderr, _("%s unable to create ~/.weechat/logs directory\n"),
                 WEECHAT_WARNING);
    }
    chmod (dir_name, 0700);
    
    free (dir_name);
}

/*
 * wee_init_vars: initialize some variables
 */

void
wee_init_vars ()
{
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
    int filename_length;
    char *filename;
    
    filename_length = strlen (weechat_home) + 64;
    filename =
        (char *) malloc (filename_length * sizeof (char));
    snprintf (filename, filename_length, "%s/" WEECHAT_LOG_NAME, weechat_home);
    if ((weechat_log_file = fopen (filename, "wt")) == NULL)
        fprintf (stderr,
                 _("%s unable to create/append to log file (~/.weechat/%s)"),
                 WEECHAT_WARNING, WEECHAT_LOG_NAME);
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
    dcc_end ();
    server_free_all ();
    gui_end ();
    if (weechat_log_file)
        fclose (weechat_log_file);
    exit (EXIT_SUCCESS);
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
    
    local_charset = strdup (nl_langinfo (CODESET));
    
    signal (SIGINT, my_sigint);     /* ignore SIGINT signal                 */
    gui_pre_init (&argc, &argv);    /* pre-initiliaze interface             */
    wee_init_vars ();               /* initialize some variables            */
    wee_parse_args (argc, argv);    /* parse command line args              */
    wee_create_home_dirs ();        /* create WeeChat directories           */
    wee_init_log ();                /* init log file                        */
    command_index_build ();         /* build commands index for completion  */
    
    switch (config_read ())         /* read configuration                   */
    {
        case 0:                     /* config file OK                       */
            break;
        case -1:                    /* config file not found                */
            if (config_create_default () < 0)
                return EXIT_FAILURE;
            if (config_read () != 0)
                return EXIT_FAILURE;
            break;
        default:                    /* other error (fatal)                  */
            server_free_all ();
            return EXIT_FAILURE;
    }
    
    gui_init ();                    /* init WeeChat interface               */
    plugin_init ();                 /* init plugin interface(s)             */    
    weechat_welcome_message ();     /* display WeeChat welcome message      */
                                    /* auto-connect to servers              */
    server_auto_connect (server_cmd_line);
    
    gui_main_loop ();               /* WeeChat main loop                    */
    
    plugin_end ();                  /* end plugin interface(s)              */
    server_disconnect_all ();       /* disconnect from all servers          */
    (void) config_write (NULL);     /* save config file                     */
    wee_shutdown ();                /* quit WeeChat (oh no, why?)           */
    
    return EXIT_SUCCESS;            /* make gcc happy (never executed)      */
}
