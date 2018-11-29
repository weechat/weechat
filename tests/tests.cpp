/*
 * tests.cpp - run WeeChat tests
 *
 * Copyright (C) 2014-2018 Sébastien Helleu <flashcode@flashtux.org>
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

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

extern "C"
{
#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H
#endif
#include "src/core/weechat.h"
#include "src/core/wee-hook.h"
#include "src/core/wee-input.h"
#include "src/core/wee-string.h"
#include "src/core/wee-util.h"
#include "src/plugins/plugin.h"
#include "src/gui/gui-main.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-chat.h"

    extern void gui_main_init ();
    extern void gui_main_loop ();
}

#include "CppUTest/CommandLineTestRunner.h"

#define LOCALE_TESTS "en_US.UTF-8"

#define WEECHAT_TESTS_HOME "./tmp_weechat_test"

/* import tests from libs */
/* core */
IMPORT_TEST_GROUP(Plugins);
IMPORT_TEST_GROUP(CoreArraylist);
IMPORT_TEST_GROUP(CoreEval);
IMPORT_TEST_GROUP(CoreHashtable);
IMPORT_TEST_GROUP(CoreHdata);
IMPORT_TEST_GROUP(CoreHook);
IMPORT_TEST_GROUP(CoreInfolist);
IMPORT_TEST_GROUP(CoreList);
IMPORT_TEST_GROUP(CoreSecure);
IMPORT_TEST_GROUP(CoreString);
IMPORT_TEST_GROUP(CoreUrl);
IMPORT_TEST_GROUP(CoreUtf8);
IMPORT_TEST_GROUP(CoreUtil);
/* GUI */
IMPORT_TEST_GROUP(GuiLine);
/* scripts */
IMPORT_TEST_GROUP(Scripts);

struct t_gui_buffer *ptr_core_buffer = NULL;


/*
 * Callback for exec_on_files (to remove all files in WeeChat home directory).
 */

void
exec_on_files_cb (void *data, const char *filename)
{
    /* make C++ compiler happy */
    (void) data;

    unlink (filename);
}

/*
 * Callback for any message displayed by WeeChat or a plugin.
 */

int
test_print_cb (const void *pointer, void *data, struct t_gui_buffer *buffer,
               time_t date, int tags_count, const char **tags, int displayed,
               int highlight, const char *prefix, const char *message)
{
    /* make C++ compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;
    (void) date;
    (void) tags_count;
    (void) tags;
    (void) displayed;
    (void) highlight;

    /* keep only messages displayed on core buffer */
    if (strcmp (gui_buffer_get_string (buffer, "full_name"), "core.weechat") != 0)
        return WEECHAT_RC_OK;

    printf ("%s%s%s\n",  /* with color: "\33[34m%s%s%s\33[0m\n" */
            (prefix && prefix[0]) ? prefix : "",
            (prefix && prefix[0]) ? " " : "",
            (message && message[0]) ? message : "");

    return WEECHAT_RC_OK;
}

/*
 * Initializes GUI for tests.
 */

void
test_gui_init ()
{
    /*
     * Catch all messages to display them directly on stdout
     * (Curses library is not used for tests).
     */
    hook_print (NULL,  /* plugin */
                NULL,  /* buffer */
                NULL,  /* tags */
                NULL,  /* message */
                1,     /* strip colors */
                &test_print_cb,
                NULL,
                NULL);

    /*
     * Call the function "gui_main_init" from Curses sources (all Curses
     * calls are made with the fake ncurses library).
     */
    gui_main_init ();
}

/*
 * Displays and runs a command on a buffer.
 */

void
run_cmd (const char *command)
{
    printf (">>> Running command: %s\n", command);
    input_data (ptr_core_buffer, command);
}

/*
 * Runs tests in WeeChat environment.
 */

int
main (int argc, char *argv[])
{
    int rc, length, weechat_argc;
    char *weechat_tests_args, *args, **weechat_argv;

    /* setup environment: English language, no specific timezone */
    setenv ("LC_ALL", LOCALE_TESTS, 1);
    setenv ("TZ", "", 1);

    /* check if locale exists */
    if (!setlocale (LC_ALL, ""))
    {
        fprintf (stderr,
                 "ERROR: the locale %s must be installed to run WeeChat "
                 "tests.\n",
                 LOCALE_TESTS);
        return 1;
    }

    /* clean WeeChat home */
    util_exec_on_files (WEECHAT_TESTS_HOME, 1, 1, &exec_on_files_cb, NULL);

    /* build arguments for WeeChat */
    weechat_tests_args = getenv ("WEECHAT_TESTS_ARGS");
    length = strlen (argv[0]) +
        64 +  /* --dir ... */
        ((weechat_tests_args) ? 1 + strlen (weechat_tests_args) : 0) +
        1;
    args = (char *)malloc (length);
    if (!args)
    {
        fprintf (stderr, "Memory error\n");
        return 1;
    }
    snprintf (args, length,
              "%s --dir %s%s%s",
              argv[0],
              WEECHAT_TESTS_HOME,
              (weechat_tests_args) ? " " : "",
              (weechat_tests_args) ? weechat_tests_args : "");
    weechat_argv = string_split_shell (args, &weechat_argc);
    printf ("WeeChat arguments: \"%s\"\n", args);

    /* init WeeChat */
    printf ("------------------------------------------------------------\n");
    weechat_init_gettext ();
    weechat_init (weechat_argc, weechat_argv, &test_gui_init);
    if (weechat_argv)
        string_free_split (weechat_argv);
    free (args);

    ptr_core_buffer = gui_buffer_search_main ();

    /* auto-load plugins from WEECHAT_EXTRA_LIBDIR if no plugin were loaded */
    if (!weechat_plugins)
    {
        gui_chat_printf (NULL,
                         "Auto-loading plugins from path in "
                         "environment variable WEECHAT_EXTRA_LIBDIR (\"%s\")",
                         getenv ("WEECHAT_EXTRA_LIBDIR"));
        plugin_auto_load (NULL, 0, 1, 0, 0, NULL);
    }

    /* display WeeChat version and directories */
    run_cmd ("/command core version");
    run_cmd ("/debug dirs");
    run_cmd ("/debug libs");

    /* run all tests */
    printf ("\n");
    printf (">>>>>>>>>> TESTS >>>>>>>>>>\n");
    rc = CommandLineTestRunner::RunAllTests (argc, argv);
    printf ("<<<<<<<<<< TESTS <<<<<<<<<<\n");
    printf ("\n");

    /* end WeeChat */
    weechat_end (&gui_main_end);
    printf ("------------------------------------------------------------\n");

    /* display status */
    printf ("\n");
    printf ("\33[%d;1m*** %s ***\33[0m\n",
            (rc == 0) ? 32 : 31,  /* 32 = green (OK), 31 = red (error) */
            (rc == 0) ? "OK" : "ERROR");

    return rc;
}
