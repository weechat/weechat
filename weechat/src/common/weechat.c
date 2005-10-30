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
 * ###                By FlashCode <flashcode@flashtux.org>                 ###
 * ###                                                                      ###
 * ###                     http://weechat.flashtux.org                      ###
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

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#ifdef HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif

#include "weechat.h"
#include "weeconfig.h"
#include "command.h"
#include "fifo.h"
#include "utf8.h"
#include "../irc/irc.h"
#include "../gui/gui.h"

#ifdef PLUGINS
#include "../plugins/plugins.h"
#endif


int quit_weechat;               /* = 1 if quit request from user... why ? :'(       */
int sigsegv = 0;                /* SIGSEGV received?                                */
char *weechat_home = NULL;      /* WeeChat home dir. (example: /home/toto/.weechat) */
FILE *weechat_log_file = NULL;  /* WeeChat log file (~/.weechat/weechat.log)        */

char *local_charset = NULL;     /* local charset, for example: ISO-8859-1, UTF-8    */

int server_cmd_line;            /* at least one server on WeeChat command line      */
int auto_connect;               /* enabled by default, can by disabled on cmd line  */
int auto_load_plugins;          /* enabled by default, can by disabled on cmd line  */

#ifdef HAVE_GNUTLS
gnutls_certificate_credentials gnutls_xcred;   /* gnutls client credentials */
#endif


/*
 * ascii_strcasecmp: locale and case independent string comparison
 */

int
ascii_strcasecmp (char *string1, char *string2)
{
    int c1, c2;
    
    if (!string1 || !string2)
        return (string1) ? 1 : ((string2) ? -1 : 0);
    
    while (string1[0] && string2[0])
    {
        c1 = (int)((unsigned char) string1[0]);
        c2 = (int)((unsigned char) string2[0]);
        
        if ((c1 >= 'A') && (c1 <= 'Z'))
            c1 += ('a' - 'A');
        
        if ((c2 >= 'A') && (c2 <= 'Z'))
            c2 += ('a' - 'A');
        
        if ((c1 - c2) != 0)
            return c1 - c2;
        
        string1++;
        string2++;
    }
    
    return (string1[0]) ? 1 : ((string2[0]) ? -1 : 0);
}

/*
 * ascii_strncasecmp: locale and case independent string comparison
 *                    with max length
 */

int
ascii_strncasecmp (char *string1, char *string2, int max)
{
    int c1, c2, count;
    
    if (!string1 || !string2)
        return (string1) ? 1 : ((string2) ? -1 : 0);
    
    count = 0;
    while ((count < max) && string1[0] && string2[0])
    {
        c1 = (int)((unsigned char) string1[0]);
        c2 = (int)((unsigned char) string2[0]);
        
        if ((c1 >= 'A') && (c1 <= 'Z'))
            c1 += ('a' - 'A');
        
        if ((c2 >= 'A') && (c2 <= 'Z'))
            c2 += ('a' - 'A');
        
        if ((c1 - c2) != 0)
            return c1 - c2;
        
        string1++;
        string2++;
        count++;
    }
    
    if (count >= max)
        return 0;
    else
        return (string1[0]) ? 1 : ((string2[0]) ? -1 : 0);
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
 * weechat_convert_encoding: convert string to another encoding
 */

char *
weechat_convert_encoding (char *from_code, char *to_code, char *string)
{
    char *outbuf;
    
    #ifdef HAVE_ICONV
    iconv_t cd;
    char *inbuf;
    ICONV_CONST char *ptr_inbuf;
    char *ptr_outbuf;
    size_t inbytesleft, outbytesleft;
    
    if (from_code && from_code[0] && to_code && to_code[0]
        && (ascii_strcasecmp(from_code, to_code) != 0))
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
    #else
    /* make gcc happy */
    (void) from_code;
    (void) to_code;
    outbuf = strdup (string);
    #endif /* HAVE_ICONV */
    
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
                printf (_("  . description: %s\n"),
                    _(weechat_options[i][j].long_description));
                printf ("\n");
                j++;
            }
        }
    }
    printf (_("Moreover, you can define aliases in [alias] section, by adding lines like:\n"));
    printf ("j=join\n");
    printf (_("where 'j' is alias name, and 'join' associated command.\n\n"));
}

/*
 * wee_display_commands: display WeeChat and/or IRC commands
 */

void wee_display_commands (int weechat_cmd, int irc_cmd)
{
    int i;
    
    if (weechat_cmd)
    {
        printf (_("%s internal commands:\n"), PACKAGE_NAME);
        printf ("\n");
        for (i = 0; weechat_commands[i].command_name; i++)
        {
            printf ("* %s", weechat_commands[i].command_name);
            if (weechat_commands[i].arguments &&
                weechat_commands[i].arguments[0])
                printf ("  %s\n\n", _(weechat_commands[i].arguments));
            else
                printf ("\n\n");
            printf ("%s\n\n", _(weechat_commands[i].command_description));
            if (weechat_commands[i].arguments_description &&
                weechat_commands[i].arguments_description[0])
                printf ("%s\n\n",
                    _(weechat_commands[i].arguments_description));
        }
    }
    
    if (irc_cmd)
    {
        printf (_("IRC commands:\n"));
        printf ("\n");
        for (i = 0; irc_commands[i].command_name; i++)
        {
            if (irc_commands[i].cmd_function_args ||
                irc_commands[i].cmd_function_1arg)
            {
                printf ("* %s", irc_commands[i].command_name);
                if (irc_commands[i].arguments &&
                    irc_commands[i].arguments[0])
                    printf ("  %s\n\n", _(irc_commands[i].arguments));
                else
                    printf ("\n\n");
                printf ("%s\n\n", _(irc_commands[i].command_description));
                if (irc_commands[i].arguments_description &&
                    irc_commands[i].arguments_description[0])
                    printf ("%s\n\n",
                        _(irc_commands[i].arguments_description));
            }
        }
    }
}

/*
 * wee_display_key_functions: display WeeChat key functions
 */

void wee_display_key_functions ()
{
    int i;
    
    printf (_("Internal key functions:\n"));
    printf ("\n");
    i = 0;
    while (gui_key_functions[i].function_name)
    {
        printf ("* %s: %s\n",
                gui_key_functions[i].function_name,
                _(gui_key_functions[i].description));
        i++;
    }
}

/*
 * wee_display_keys: display WeeChat default keys
 */

void
wee_display_keys ()
{
    t_gui_key *ptr_key;
    char *expanded_name;
    
    printf (_("%s default keys:\n"), PACKAGE_NAME);
    printf ("\n");
    for (ptr_key = gui_keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        expanded_name = gui_key_get_expanded_name (ptr_key->key);
        printf ("* %s => %s\n",
                (expanded_name) ? expanded_name : ptr_key->key,
                (ptr_key->function) ? gui_key_function_search_by_ptr (ptr_key->function) : ptr_key->command);
        if (expanded_name)
            free (expanded_name);
    }
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
            wee_display_config_options ();
            wee_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((strcmp (argv[i], "-f") == 0)
            || (strcmp (argv[i], "--key-functions") == 0))
        {
            wee_display_key_functions ();
            wee_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((strcmp (argv[i], "-h") == 0)
                || (strcmp (argv[i], "--help") == 0))
        {
            printf ("\n" WEE_USAGE1, argv[0], argv[0]);
            printf ("%s", WEE_USAGE2);
            wee_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((strcmp (argv[i], "-i") == 0)
            || (strcmp (argv[i], "--irc-commands") == 0))
        {
            wee_display_commands (0, 1);
            wee_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((strcmp (argv[i], "-k") == 0)
            || (strcmp (argv[i], "--keys") == 0))
        {
            wee_display_keys ();
            wee_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((strcmp (argv[i], "-l") == 0)
                 || (strcmp (argv[i], "--license") == 0))
        {
            printf ("\n%s%s", WEE_LICENSE);
            wee_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((strcmp (argv[i], "-p") == 0)
                 || (strcmp (argv[i], "--no-plugin") == 0))
            auto_load_plugins = 0;
        else if ((strcmp (argv[i], "-v") == 0)
                 || (strcmp (argv[i], "--version") == 0))
        {
            printf (PACKAGE_VERSION "\n");
            wee_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((strcmp (argv[i], "-w") == 0)
            || (strcmp (argv[i], "--weechat-commands") == 0))
        {
            wee_display_commands (1, 0);
            wee_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((ascii_strncasecmp (argv[i], "irc", 3) == 0))
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
                                 server_tmp.ipv6, server_tmp.ssl,
                                 server_tmp.password, server_tmp.nick1,
                                 server_tmp.nick2, server_tmp.nick3,
                                 NULL, NULL, NULL, 0, server_tmp.autojoin, 1, NULL))
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
        wee_shutdown (EXIT_FAILURE, 0);
    }
    dir_length = strlen (ptr_home) + 10;
    weechat_home =
        (char *) malloc (dir_length * sizeof (char));
    if (!weechat_home)
    {
        fprintf (stderr, _("%s not enough memory for home directory\n"),
                 WEECHAT_ERROR);
        wee_shutdown (EXIT_FAILURE, 0);
    }
    snprintf (weechat_home, dir_length, "%s%s.weechat", ptr_home,
              DIR_SEPARATOR);
    
    /* create home directory "~/.weechat" ; error is fatal */
    if (!wee_create_dir (weechat_home))
    {
        fprintf (stderr, _("%s unable to create ~/.weechat directory\n"),
                 WEECHAT_ERROR);
        wee_shutdown (EXIT_FAILURE, 0);
    }
    
    dir_length = strlen (weechat_home) + 64;
    dir_name = (char *) malloc (dir_length * sizeof (char));
    
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
    
    /* init gnutls */
#ifdef HAVE_GNUTLS
    gnutls_global_init ();
    gnutls_certificate_allocate_credentials (&gnutls_xcred);
    gnutls_certificate_set_x509_trust_file (gnutls_xcred, "ca.pem", GNUTLS_X509_FMT_PEM);
#endif
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
    
    wee_log_printf ("%s (%s %s %s)\n",
                    PACKAGE_STRING, _("compiled on"), __DATE__, __TIME__);
}

/*
 * wee_gui_shutdown: shutdown WeeChat GUI
 */

void
wee_gui_shutdown ()
{
    dcc_end ();
    server_free_all ();
    gui_end ();
}

/*
 * wee_shutdown: shutdown WeeChat
 */

void
wee_shutdown (int return_code, int crash)
{
    fifo_remove ();
    if (weechat_home)
        free (weechat_home);
    if (weechat_log_file)
        fclose (weechat_log_file);
    if (local_charset)
        free (local_charset);
    alias_free_all ();
    
#ifdef HAVE_GNUTLS
    gnutls_certificate_free_credentials (gnutls_xcred);
    gnutls_global_deinit();
#endif
    
    if (crash)
        abort();
    else
        exit (return_code);
}

/*
 * wee_dump writes dump to WeeChat log file
 */

void
wee_dump (int crash)
{
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    t_gui_window *ptr_window;
    t_gui_buffer *ptr_buffer;
    
    /* prevent reentrance */
    if (sigsegv)
        exit (EXIT_FAILURE);
    
    if (crash)
    {
        sigsegv = 1;
        wee_log_printf ("Very bad, WeeChat is crashing (SIGSEGV received)...\n");
    }
    
    wee_log_printf ("\n");
    if (crash)
    {
        wee_log_printf ("******             WeeChat CRASH DUMP              ******\n");
        wee_log_printf ("****** Please send this file to WeeChat developers ******\n");
        wee_log_printf ("******    and explain when this crash happened     ******\n");
    }
    else
    {
        wee_log_printf ("******            WeeChat dump request             ******\n");
    }
    
    for (ptr_server = irc_servers; ptr_server; ptr_server = ptr_server->next_server)
    {
        wee_log_printf ("\n");
        server_print_log (ptr_server);
        
        for (ptr_channel = ptr_server->channels; ptr_channel;
            ptr_channel = ptr_channel->next_channel)
        {
            wee_log_printf ("\n");
            channel_print_log (ptr_channel);
            
            for (ptr_nick = ptr_channel->nicks; ptr_nick;
                ptr_nick = ptr_nick->next_nick)
            {
                wee_log_printf ("\n");
                nick_print_log (ptr_nick);
            }
            
        }
    }
    
    wee_log_printf ("\n");
    wee_log_printf ("[windows/buffers]\n");
    wee_log_printf ("  => windows:\n");
    for (ptr_window = gui_windows; ptr_window; ptr_window = ptr_window->next_window)
    {
        wee_log_printf ("       0x%X\n", ptr_window);
    }
    wee_log_printf ("  => buffers:\n");
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        wee_log_printf ("       0x%X\n", ptr_buffer);
    }
    wee_log_printf ("  => current window = 0x%X\n", gui_current_window);
    
    for (ptr_window = gui_windows; ptr_window; ptr_window = ptr_window->next_window)
    {
        wee_log_printf ("\n");
        gui_window_print_log (ptr_window);
    }
    
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        wee_log_printf ("\n");
        gui_buffer_print_log (ptr_buffer);
    }

    wee_log_printf ("\n");
    ignore_print_log ();
    
    wee_log_printf ("\n");
    wee_log_printf ("******                 End of dump                 ******\n");
    wee_log_printf ("\n");
}

/*
 * my_sigsegv: SIGSEGV handler: save crash log to ~/.weechat/weechat.log and exit
 */

void
my_sigsegv ()
{
    wee_dump (1);
    wee_gui_shutdown ();
    fprintf (stderr, "\n");
    fprintf (stderr, "*** Very bad! WeeChat has crashed (SIGSEGV received)\n");
    fprintf (stderr, "*** Full crash dump was saved to ~/.weechat/weechat.log file\n");
    fprintf (stderr, "*** Please send this file to WeeChat developers.\n");
    fprintf (stderr, "*** (be careful, private info may be in this file since\n");
    fprintf (stderr, "*** part of chats are displayed, so remove lines if needed)\n\n");
    wee_shutdown (EXIT_FAILURE, 1);
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
    
    #ifdef HAVE_LANGINFO_CODESET
    local_charset = strdup (nl_langinfo (CODESET));
    #endif
    
    signal (SIGINT, SIG_IGN);       /* ignore SIGINT signal                 */
    signal (SIGQUIT, SIG_IGN);      /* ignore SIGQUIT signal                */
    signal (SIGPIPE, SIG_IGN);      /* ignore SIGPIPE signal                */
    signal (SIGSEGV, my_sigsegv);   /* crash dump when SIGSEGV is received  */
    gui_pre_init (&argc, &argv);    /* pre-initiliaze interface             */
    wee_init_vars ();               /* initialize some variables            */
    gui_key_init ();                /* init keyboard (default key bindings) */
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
    
    utf8_init ();                   /* init UTF-8 in WeeChat                */
    gui_init ();                    /* init WeeChat interface               */
    weechat_welcome_message ();     /* display WeeChat welcome message      */
#ifdef PLUGINS
    plugin_init (auto_load_plugins);/* init plugin interface(s)             */
#endif
                                    /* auto-connect to servers              */
    server_auto_connect (auto_connect, server_cmd_line);
    fifo_create ();                 /* create FIFO pipe for remote control  */
    
    gui_main_loop ();               /* WeeChat main loop                    */
    
#ifdef PLUGINS    
    plugin_end ();                  /* end plugin interface(s)              */
#endif
    server_disconnect_all ();       /* disconnect from all servers          */
    (void) config_write (NULL);     /* save config file                     */
    command_index_free ();          /* free commands index                  */
    wee_gui_shutdown ();            /* shut down WeeChat GUI                */
    wee_shutdown (EXIT_SUCCESS, 0); /* quit WeeChat (oh no, why?)           */
    
    return EXIT_SUCCESS;            /* make gcc happy (never executed)      */
}
