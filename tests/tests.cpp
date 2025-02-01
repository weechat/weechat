/*
 * tests.cpp - run WeeChat tests
 *
 * Copyright (C) 2014-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <dlfcn.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "tests-record.h"

extern "C"
{
#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H
#endif
#include "src/core/weechat.h"
#include "src/core/core-arraylist.h"
#include "src/core/core-dir.h"
#include "src/core/core-hook.h"
#include "src/core/core-input.h"
#include "src/core/core-string.h"
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
IMPORT_TEST_GROUP(CoreArraylist);
IMPORT_TEST_GROUP(CoreCalc);
IMPORT_TEST_GROUP(CoreCommand);
IMPORT_TEST_GROUP(CoreConfigFile);
IMPORT_TEST_GROUP(CoreCrypto);
IMPORT_TEST_GROUP(CoreDir);
IMPORT_TEST_GROUP(CoreEval);
IMPORT_TEST_GROUP(CoreHashtable);
IMPORT_TEST_GROUP(CoreHdata);
IMPORT_TEST_GROUP(CoreHook);
IMPORT_TEST_GROUP(CoreInfolist);
IMPORT_TEST_GROUP(CoreInput);
IMPORT_TEST_GROUP(CoreList);
IMPORT_TEST_GROUP(CoreNetwork);
IMPORT_TEST_GROUP(CoreSecure);
IMPORT_TEST_GROUP(CoreSignal);
IMPORT_TEST_GROUP(CoreString);
IMPORT_TEST_GROUP(CoreUrl);
IMPORT_TEST_GROUP(CoreUtf8);
IMPORT_TEST_GROUP(CoreUtil);
IMPORT_TEST_GROUP(CoreSys);
/* core/hook */
IMPORT_TEST_GROUP(HookCommand);
IMPORT_TEST_GROUP(HookCommandRun);
IMPORT_TEST_GROUP(HookCompletion);
IMPORT_TEST_GROUP(HookConfig);
IMPORT_TEST_GROUP(HookConnect);
IMPORT_TEST_GROUP(HookFd);
IMPORT_TEST_GROUP(HookFocus);
IMPORT_TEST_GROUP(HookHdata);
IMPORT_TEST_GROUP(HookHsignal);
IMPORT_TEST_GROUP(HookInfo);
IMPORT_TEST_GROUP(HookInfoHashtable);
IMPORT_TEST_GROUP(HookInfolist);
IMPORT_TEST_GROUP(HookLine);
IMPORT_TEST_GROUP(HookModifier);
IMPORT_TEST_GROUP(HookPrint);
IMPORT_TEST_GROUP(HookProcess);
IMPORT_TEST_GROUP(HookSignal);
IMPORT_TEST_GROUP(HookTimer);
IMPORT_TEST_GROUP(HookUrl);
/* GUI */
IMPORT_TEST_GROUP(GuiBar);
IMPORT_TEST_GROUP(GuiBarItem);
IMPORT_TEST_GROUP(GuiBarItemCustom);
IMPORT_TEST_GROUP(GuiBarWindow);
IMPORT_TEST_GROUP(GuiBuffer);
IMPORT_TEST_GROUP(GuiChat);
IMPORT_TEST_GROUP(GuiColor);
IMPORT_TEST_GROUP(GuiFilter);
IMPORT_TEST_GROUP(GuiHotlist);
IMPORT_TEST_GROUP(GuiInput);
IMPORT_TEST_GROUP(GuiKey);
IMPORT_TEST_GROUP(GuiLine);
IMPORT_TEST_GROUP(GuiNick);
IMPORT_TEST_GROUP(GuiNicklist);
/* GUI - Curses */
IMPORT_TEST_GROUP(GuiCursesMouse);
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
 * Callback for any message displayed by WeeChat or a plugin (colors stripped).
 */

int
test_print_cb (const void *pointer, void *data, struct t_gui_buffer *buffer,
               time_t date, int date_usec, int tags_count, const char **tags,
               int displayed, int highlight,
               const char *prefix, const char *message)
{
    /* make C++ compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;
    (void) date;
    (void) date_usec;
    (void) tags_count;
    (void) tags;
    (void) displayed;
    (void) highlight;

    /* keep only messages displayed on core buffer */
    if (strcmp (gui_buffer_get_string (buffer, "full_name"), "core.weechat") == 0)
    {
        printf ("%s%s%s\n",  /* with color: "\33[34m%s%s%s\33[0m\n" */
                (prefix && prefix[0]) ? prefix : "",
                (prefix && prefix[0] && message && message[0]) ? " " : "",
                (message && message[0]) ? message : "");
    }

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
    input_data (ptr_core_buffer, command, NULL, 0, 0);
}

/*
 * Runs a command on a buffer (do not display the command executed).
 */

void
run_cmd_quiet (const char *command)
{
    input_data (ptr_core_buffer, command, NULL, 0, 0);
}

/*
 * Runs tests in WeeChat environment.
 */

int
main (int argc, char *argv[])
{
    int rc, length, weechat_argc;
    char *weechat_tests_args, *args, **weechat_argv, *tests_plugins_lib;
    const char *ptr_path;
    void *handle;

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
    dir_exec_on_files (WEECHAT_TESTS_HOME, 1, 1, &exec_on_files_cb, NULL);

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

    /* load plugins tests */
    tests_plugins_lib = getenv ("WEECHAT_TESTS_PLUGINS_LIB");
    ptr_path = (tests_plugins_lib && tests_plugins_lib[0]) ?
        tests_plugins_lib : NULL;
    if (!ptr_path)
    {
        fprintf (stderr,
                 "ERROR: environment variable WEECHAT_TESTS_PLUGINS_LIB "
                 "is not defined\n");
        return 1;
    }
    printf ("Loading tests on plugins: \"%s\"\n", ptr_path);
    handle = dlopen (ptr_path, RTLD_GLOBAL | RTLD_NOW);
    if (!handle)
    {
        fprintf (stderr, "ERROR: unable to load tests on plugins: %s\n",
                 dlerror ());
        return 1;
    }

    /* display WeeChat version and directories */
    run_cmd ("/command core version");
    run_cmd ("/debug dirs");
    run_cmd ("/debug libs");

    /* run all tests */
    printf ("\n");
    rc = CommandLineTestRunner::RunAllTests (argc, argv);

    /* end WeeChat */
    gui_chat_mute = GUI_CHAT_MUTE_ALL_BUFFERS;
    weechat_end (&gui_main_end);

    dlclose (handle);

    return rc;
}
