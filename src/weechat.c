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
 * Copyright (c) 2003 by FlashCode <flashcode@flashtux.org>
 *                       Bounga <bounga@altern.org>
 *                       Xahlexx <xahlexx@tuxisland.org>
 * See README for License detail.
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


#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "weechat.h"
#include "config.h"
#include "command.h"
#include "irc/irc.h"
#include "gui/gui.h"


/* char *display_name; */
int quit_weechat;           /* = 1 if quit request from user... why ? :'(   */

FILE *log_file;             /* WeeChat log file (~/.weechat/weechat.log     */


/*
 * log_printf: displays a message in WeeChat log (~/.weechat/weechat.log)
 */

void
log_printf (char *message, ...)
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
    int i;

    for (i = 1; i < argc; i++)
    {
        if ((strcmp (argv[i], "-h") == 0)
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
        /*else if ((strcmp (argv[i], "-d") == 0)
                 || (strcmp (argv[i], "--display") == 0))
        {
            if (i == (argc - 1))
                fprintf (stderr,
                         _("%s no display specified (parameter '%s'), ignored\n"),
                         WEECHAT_WARNING, argv[i]);
            else
            {
                display_name = argv[i + 1];
                i++;
            }
        }*/
        else if ((strcmp (argv[i], "-v") == 0)
                 || (strcmp (argv[i], "--version") == 0))
        {
            printf (WEECHAT_VERSION "\n");
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
 * wee_create_home_dir: create weechat home directory (if not found)
 */

void
wee_create_home_dir ()
{
    char *weechat_home_dir;
    int return_code;

    weechat_home_dir =
        (char *) malloc ((strlen (getenv ("HOME")) + 64) * sizeof (char));
    sprintf (weechat_home_dir, "%s/.weechat", getenv ("HOME"));
    return_code = mkdir (weechat_home_dir, 0755);
    if (return_code < 0)
    {
        if (errno != EEXIST)
        {
            fprintf (stderr, _("%s cannot create directory \"%s\"\n"),
                     WEECHAT_ERROR, weechat_home_dir);
            free (weechat_home_dir);
            exit (1);
        }
    }
    free (weechat_home_dir);
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
        (char *) malloc ((strlen (getenv ("HOME")) + 64) * sizeof (char));
    sprintf (filename, "%s/.weechat/" WEECHAT_LOG_NAME, getenv ("HOME"));
    if ((log_file = fopen (filename, "wt")) == NULL)
    {
        free (filename);
        fprintf (stderr,
                 _("%s unable to create/append to log file (~/.weechat/"
                 WEECHAT_LOG_NAME), WEECHAT_ERROR);
    }
    free (filename);
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
    t_irc_server *ptr_server;
    
    /* initialize variables */
    wee_init_vars ();
    
    /* parse command line args */
    wee_parse_args (argc, argv);

    /* create weechat home directory */
    wee_create_home_dir ();
    
    /* init log file */
    wee_init_log ();
    
    /* read configuration */
    switch (config_read ())
    {
        case 0:                    /* success */
            break;
        case -1:                   /* config file not found */
            config_create_default ();
            config_read ();
            break;
        default:                   /* other error (fatal) */
            server_free_all ();
            return 1;
    }
    
    /* init gui */
    gui_init ();
    
    /* build commands index (sorted), for completion */
    index_command_build ();
    
    /* Welcome message - yeah! */
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
        gui_printf_color (NULL, COLOR_WIN_CHAT_PREFIX2, WEECHAT_NAME);
        gui_printf_color (NULL, COLOR_WIN_CHAT,
                          ", %s\n", cfg_look_weechat_slogan);
    }
    if (cfg_look_startup_version)
    {
        gui_printf_color (NULL, COLOR_WIN_CHAT_PREFIX2,
                          "%s" WEECHAT_NAME_AND_VERSION,
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
    
    /* connect to all servers (with autoconnect flag) */
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->autoconnect)
        {
            gui_window_new (ptr_server, NULL);
            if (server_connect (ptr_server))
                irc_login (ptr_server);
        }
    }
    gui_main_loop ();
    server_disconnect_all ();

    /* save config file */
    config_write (NULL);
    
    /* program ending */
    wee_shutdown ();

    /* make gcc happy (statement never executed) */
    return 0;
}
