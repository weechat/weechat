/*
 * tests.cpp - run WeeChat tests
 *
 * Copyright (C) 2014-2023 Sébastien Helleu <flashcode@flashtux.org>
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

extern "C"
{
#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H
#endif
#include "src/core/weechat.h"
#include "src/core/wee-arraylist.h"
#include "src/core/wee-dir.h"
#include "src/core/wee-hook.h"
#include "src/core/wee-input.h"
#include "src/core/wee-string.h"
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

/* lib with tests on plugins when autotools is used to compile */
#define WEECHAT_TESTS_PLUGINS_LIB_DEFAULT                       \
    "./tests/.libs/lib_weechat_unit_tests_plugins.so.0.0.0"

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
IMPORT_TEST_GROUP(CoreList);
IMPORT_TEST_GROUP(CoreNetwork);
IMPORT_TEST_GROUP(CoreSecure);
IMPORT_TEST_GROUP(CoreSignal);
IMPORT_TEST_GROUP(CoreString);
IMPORT_TEST_GROUP(CoreUrl);
IMPORT_TEST_GROUP(CoreUtf8);
IMPORT_TEST_GROUP(CoreUtil);
/* GUI */
IMPORT_TEST_GROUP(GuiBarItem);
IMPORT_TEST_GROUP(GuiBarItemCustom);
IMPORT_TEST_GROUP(GuiBarWindow);
IMPORT_TEST_GROUP(GuiBuffer);
IMPORT_TEST_GROUP(GuiChat);
IMPORT_TEST_GROUP(GuiColor);
IMPORT_TEST_GROUP(GuiFilter);
IMPORT_TEST_GROUP(GuiInput);
IMPORT_TEST_GROUP(GuiLine);
IMPORT_TEST_GROUP(GuiNick);
/* scripts */
IMPORT_TEST_GROUP(Scripts);

struct t_gui_buffer *ptr_core_buffer = NULL;

/* recording of messages: to test if a message is actually displayed */
int record_messages = 0;
struct t_arraylist *recorded_messages = NULL;


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
               time_t date, int tags_count, const char **tags, int displayed,
               int highlight, const char *prefix, const char *message)
{
    const char *buffer_full_name;
    char str_recorded[8192];

    /* make C++ compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;
    (void) date;
    (void) tags_count;
    (void) tags;
    (void) displayed;
    (void) highlight;

    buffer_full_name = gui_buffer_get_string (buffer, "full_name");

    if (record_messages)
    {
        snprintf (str_recorded, sizeof (str_recorded),
                  "%s: \"%s%s%s\"",
                  buffer_full_name,
                  (prefix && prefix[0]) ? prefix : "",
                  (prefix && prefix[0] && message && message[0]) ? " " : "",
                  (message && message[0]) ? message : "");
        arraylist_add (recorded_messages, strdup (str_recorded));
    }

    /* keep only messages displayed on core buffer */
    if (strcmp (buffer_full_name, "core.weechat") == 0)
    {
        printf ("%s%s%s\n",  /* with color: "\33[34m%s%s%s\33[0m\n" */
                (prefix && prefix[0]) ? prefix : "",
                (prefix && prefix[0] && message && message[0]) ? " " : "",
                (message && message[0]) ? message : "");
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback used to compare two recorded messages.
 */

int
record_cmp_cb (void *data, struct t_arraylist *arraylist,
               void *pointer1, void *pointer2)
{
    /* make C++ compiler happy */
    (void) data;
    (void) arraylist;

    return strcmp ((char *)pointer1, (char *)pointer2);
}

/*
 * Callback used to free a recorded message.
 */

void
record_free_cb (void *data, struct t_arraylist *arraylist, void *pointer)
{
    /* make C++ compiler happy */
    (void) data;
    (void) arraylist;

    free (pointer);
}

/*
 * Starts recording of messages displayed.
 */

void
record_start ()
{
    record_messages = 1;

    if (recorded_messages)
    {
        arraylist_clear (recorded_messages);
    }
    else
    {
        recorded_messages = arraylist_new (16, 0, 1,
                                           &record_cmp_cb, NULL,
                                           &record_free_cb, NULL);
    }
}

/*
 * Stops recording of messages displayed.
 */

void
record_stop ()
{
    record_messages = 0;
}

/*
 * Searches if a message has been displayed in a buffer.
 *
 * The format of "message" argument is: "prefix message" (prefix and message
 * separated by a space).
 *
 * Returns:
 *   1: message has been displayed
 *   0: message has NOT been displayed
 */

int
record_search (const char *buffer, const char *message)
{
    char str_message[8192];

    snprintf (str_message, sizeof (str_message),
              "%s: \"%s\"",
              buffer, message);

    return (arraylist_search (recorded_messages, str_message,
                              NULL, NULL) != NULL);
}

/*
 * Adds all recorded messages to the dynamic string "msg".
 */

void
record_dump (char **msg)
{
    int i;

    for (i = 0; i < arraylist_size (recorded_messages); i++)
    {
        string_dyn_concat (msg, "  ", -1);
        string_dyn_concat (msg,
                           (const char *)arraylist_get (recorded_messages, i),
                           -1);
        string_dyn_concat (msg, "\n", -1);
    }
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
    input_data (ptr_core_buffer, command, NULL);
}

/*
 * Runs a command on a buffer (do not display the command executed).
 */

void
run_cmd_quiet (const char *command)
{
    input_data (ptr_core_buffer, command, NULL);
}

/*
 * Runs tests in WeeChat environment.
 */

int
main (int argc, char *argv[])
{
    int rc, length, weechat_argc;
    char *weechat_tests_args, *args, **weechat_argv, *tests_plugins_lib;
    const char *tests_plugins_lib_default = WEECHAT_TESTS_PLUGINS_LIB_DEFAULT;
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
        tests_plugins_lib : tests_plugins_lib_default;
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
