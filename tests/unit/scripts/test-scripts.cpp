/*
 * SPDX-FileCopyrightText: 2017-2026 Sébastien Helleu <flashcode@flashtux.org>
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

/* Test scripting API */

#include "CppUTest/TestHarness.h"

#include "tests.h"
#include "tests-record.h"

extern "C"
{
#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H
#endif
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "src/core/weechat.h"
#include "src/core/core-dir.h"
#include "src/core/core-hdata.h"
#include "src/core/core-string.h"
#include "src/core/core-hook.h"
#include "src/core/core-util.h"
#include "src/plugins/plugin.h"
}

struct t_hook *api_hook_print = NULL;
int api_tests_ok = 0;
int api_tests_errors = 0;
int api_tests_count = 0;
int api_tests_end = 0;
int api_tests_other = 0;

/*
 * Write content in a file.
 *
 * Return:
 *   1: OK
 *   0: error
 */

int
test_scripts_write_file (const char *path, const char *content)
{
    FILE *file;
    int rc;

    file = fopen (path, "w");
    if (!file)
        return 0;
    rc = (fputs (content, file) >= 0);
    if (fclose (file) != 0)
        rc = 0;

    return rc;
}

/*
 * Check if a script is loaded.
 *
 * Return:
 *   1: script is loaded
 *   0: script is not loaded
 */

int
test_scripts_is_loaded (const char *language, const char *name)
{
    struct t_hdata *hdata;
    char str_hdata[128], str_search[256];

    snprintf (str_hdata, sizeof (str_hdata), "%s_script", language);
    hdata = hook_hdata_get (NULL, str_hdata);
    if (!hdata)
        return 0;
    snprintf (str_search, sizeof (str_search),
              "${%s_script.name} == %s", language, name);
    return (hdata_search (hdata, hdata_get_list (hdata, "scripts"),
                          str_search, NULL, NULL, NULL, 1) != NULL) ? 1 : 0;
}

/*
 * Run the timers until the script is unloaded, or until timeout (in
 * milliseconds) is reached.
 */

void
test_scripts_wait_unload (const char *language, const char *name, int timeout)
{
    struct timeval time_start, time_now;

    gettimeofday (&time_start, NULL);
    while (test_scripts_is_loaded (language, name))
    {
        hook_timer_exec ();
        gettimeofday (&time_now, NULL);
        if (util_timeval_diff (&time_start, &time_now) >= (long long)timeout * 1000)
            break;
    }
}

TEST_GROUP(Scripts)
{
    /*
     * Callback for any message displayed by WeeChat or a plugin.
     */

    static int
    test_print_cb (const void *pointer, void *data, struct t_gui_buffer *buffer,
                   time_t date, int date_usec, int tags_count, const char **tags,
                   int displayed, int highlight,
                   const char *prefix, const char *message)
    {
        const char *pos;
        char *error;
        int value;

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
        (void) prefix;

        if (message)
        {
            pos = strstr (message, "> TESTS: ");
            if (pos)
            {
                error = NULL;
                value = (int)strtol (pos + 9, &error, 10);
                if (error && !error[0])
                    api_tests_count = value;
            }
            else if (strstr (message, "TEST OK"))
                api_tests_ok++;
            else if (strstr (message, "ERROR"))
                api_tests_errors++;
            else if (strstr (message, "TESTS END"))
                api_tests_end++;
            else if ((message[0] != '>') && (message[0] != ' ')
                     && (strncmp (message, "## ", 3) != 0))
            {
                api_tests_other++;
            }
        }

        return WEECHAT_RC_OK;
    }

    void setup()
    {
        api_hook_print = hook_print (NULL,  /* plugin */
                                     NULL,  /* buffer */
                                     NULL,  /* tags */
                                     NULL,  /* message */
                                     1,     /* strip colors */
                                     &test_print_cb,
                                     NULL,
                                     NULL);
    }

    void teardown()
    {
        unhook (api_hook_print);
    }
};

/*
 * Tests scripting API.
 */

TEST(Scripts, API)
{
    char path_testapigen[PATH_MAX], path_testapi[PATH_MAX];
    char *path_testapi_output_dir, str_command[(PATH_MAX * 2) + 128];
    char *test_scripts_dir, str_condition[128], str_error[128];
    struct timeval time_start, time_end;
    long long diff;
    const char *ptr_test_scripts_dir;
    const char *languages[][2] = {
#ifdef HAVE_PYTHON
        { "python",     "py"  },
#endif
#ifdef HAVE_PERL
        { "perl",       "pl"  },
#endif
#ifdef HAVE_RUBY
        { "ruby",       "rb"  },
#endif
#ifdef HAVE_LUA
        { "lua",        "lua" },
#endif
#ifdef HAVE_TCL
        { "tcl",        "tcl" },
#endif
#ifdef HAVE_GUILE
        { "guile",      "scm" },
#endif
#ifdef HAVE_JAVASCRIPT
        { "javascript", "js"  },
#endif
#ifdef HAVE_PHP
        { "php",        "php" },
#endif
        { NULL,         NULL  }
    };
    int i, turnoff_memleak;
    struct t_hdata *hdata;
    void *plugins;

    printf ("...\n");

    ptr_test_scripts_dir = getenv ("WEECHAT_TESTS_SCRIPTS_DIR");
    test_scripts_dir = strdup (
        (ptr_test_scripts_dir) ?
        ptr_test_scripts_dir : "./scripts/python");

    /* build paths for scripting API tests */
    snprintf (path_testapigen, sizeof (path_testapigen),
              "%s%s%s",
              test_scripts_dir,
              DIR_SEPARATOR,
              "testapigen.py");
    snprintf (path_testapi, sizeof (path_testapi),
              "%s%s%s",
              test_scripts_dir,
              DIR_SEPARATOR,
              "testapi.py");
    path_testapi_output_dir = string_eval_path_home (
        "${weechat_data_dir}/testapi",
        NULL, NULL, NULL);
    CHECK(path_testapi_output_dir);

    api_tests_ok = 0;
    api_tests_errors = 0;

    /* load generator script */
    snprintf (str_command, sizeof (str_command),
              "/script load %s", path_testapigen);
    run_cmd (str_command);

    /* generate scripts to test API */
    snprintf (str_command, sizeof (str_command),
              "/testapigen %s %s",
              path_testapi,
              path_testapi_output_dir);
    run_cmd (str_command);

    /* check that there was no errors in script generation */
    LONGS_EQUAL(0, api_tests_errors);

    /* unload generator script */
    snprintf (str_command, sizeof (str_command),
              "/script unload testapigen.py");
    run_cmd (str_command);

    hdata = hook_hdata_get (NULL, "plugin");
    plugins = hdata_get_list (hdata, "weechat_plugins");

    /* test the scripting API */
    for (i = 0; languages[i][0]; i++)
    {
        /* test if the plugin is loaded */
        snprintf (str_condition, sizeof (str_condition),
                  "${plugin.name} == %s",
                  languages[i][0]);
        if (!hdata_search (hdata, plugins, str_condition, NULL, NULL, NULL, 1))
        {
            /* plugin not loaded */
            snprintf (str_error, sizeof (str_error),
                      "Plugin \"%s\" is not loaded",
                      languages[i][0]);
            FAIL(str_error);
        }

        /*
         * TODO: fix memory leaks in javascript plugin
         * and keep memory leak detection enabled
         */
        turnoff_memleak = (strcmp (languages[i][0], "javascript") == 0);

        if (turnoff_memleak)
            MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();

        api_tests_ok = 0;
        api_tests_errors = 0;
        api_tests_count = 0;
        api_tests_end = 0;
        api_tests_other = 0;

        /* load script (run tests) */
        snprintf (str_command, sizeof (str_command),
                  "/script load -q %s/weechat_testapi.%s",
                  path_testapi_output_dir,
                  languages[i][1]);
        run_cmd (str_command);

        /* get date/time before running tests */
        gettimeofday (&time_start, NULL);

        /* run tests */
        snprintf (str_command, sizeof (str_command),
                  "/weechat_testapi.%s",
                  languages[i][1]);
        run_cmd (str_command);

        /* compute elapsed time */
        gettimeofday (&time_end, NULL);
        diff = util_timeval_diff (&time_start, &time_end);

        /* display results */
        printf ("\n");
        printf (">>> Tests %s: %d tests, %d OK, %d errors, "
                "%d unexpected messages, %lld ms\n",
                languages[i][0],
                api_tests_count,
                api_tests_ok,
                api_tests_errors,
                api_tests_other,
                diff / 1000);
        printf ("\n");

        /* unload script */
        snprintf (str_command, sizeof (str_command),
                  "/script unload -q weechat_testapi.%s",
                  languages[i][1]);
        run_cmd (str_command);

        /* check that tests were found in script */
        CHECK(api_tests_count > 0);

        /* check that all tests are OK */
        LONGS_EQUAL(api_tests_count, api_tests_ok);

        /* check that there was no errors */
        LONGS_EQUAL(0, api_tests_errors);

        /* check that end of script was reached (no syntax error) */
        LONGS_EQUAL(1, api_tests_end);

        /*
         * check that there was no warning/error from plugin
         * (if everything is OK, there are 2 messages when the script is loaded
         * and 2 messages when it is unloaded, so total is 4)
         */
        LONGS_EQUAL(0, api_tests_other);

        if (turnoff_memleak)
            MemoryLeakWarningPlugin::turnOnThreadSafeNewDeleteOverloads();
    }

    free (path_testapi_output_dir);
    free (test_scripts_dir);

    printf ("TEST(Scripts, API)");
}

/*
 * Tests unload of a script by itself, in one of its callbacks: the unload is
 * delayed until the callback has returned (unloading the script while its
 * callback is running would crash WeeChat).
 */

TEST(Scripts, UnloadRunning)
{
    /* language, extension, code of script */
    const char *languages[][3] = {
#ifdef HAVE_PYTHON
        { "python", "py",
          "import weechat\n"
          "weechat.register('unload_running', 'me', '0.1', 'GPL3', '', '', '')\n"
          "def cmd_cb(data, buffer, args):\n"
          "    weechat.command('', '/python unload unload_running')\n"
          "    weechat.prnt('', 'unload_running: after unload')\n"
          "    return weechat.WEECHAT_RC_OK\n"
          "weechat.hook_command('unload_running_cmd', '', '', '', '', "
          "'cmd_cb', '')\n" },
#endif
#ifdef HAVE_PERL
        { "perl", "pl",
          "weechat::register('unload_running', 'me', '0.1', 'GPL3', '', '', '');\n"
          "sub cmd_cb {\n"
          "    weechat::command('', '/perl unload unload_running');\n"
          "    weechat::print('', 'unload_running: after unload');\n"
          "    return weechat::WEECHAT_RC_OK;\n"
          "}\n"
          "weechat::hook_command('unload_running_cmd', '', '', '', '', "
          "'cmd_cb', '');\n" },
#endif
#ifdef HAVE_RUBY
        { "ruby", "rb",
          "def weechat_init\n"
          "  Weechat.register('unload_running', 'me', '0.1', 'GPL3', '', '', '')\n"
          "  Weechat.hook_command('unload_running_cmd', '', '', '', '', "
          "'cmd_cb', '')\n"
          "  return Weechat::WEECHAT_RC_OK\n"
          "end\n"
          "def cmd_cb(data, buffer, args)\n"
          "  Weechat.command('', '/ruby unload unload_running')\n"
          "  Weechat.print('', 'unload_running: after unload')\n"
          "  return Weechat::WEECHAT_RC_OK\n"
          "end\n" },
#endif
#ifdef HAVE_LUA
        { "lua", "lua",
          "weechat.register('unload_running', 'me', '0.1', 'GPL3', '', '', '')\n"
          "function cmd_cb(data, buffer, args)\n"
          "  weechat.command('', '/lua unload unload_running')\n"
          "  weechat.print('', 'unload_running: after unload')\n"
          "  return weechat.WEECHAT_RC_OK\n"
          "end\n"
          "weechat.hook_command('unload_running_cmd', '', '', '', '', "
          "'cmd_cb', '')\n" },
#endif
#ifdef HAVE_TCL
        { "tcl", "tcl",
          "weechat::register unload_running me 0.1 GPL3 {} {} {}\n"
          "proc cmd_cb {data buffer arguments} {\n"
          "  weechat::command {} {/tcl unload unload_running}\n"
          "  weechat::print {} {unload_running: after unload}\n"
          "  return $::weechat::WEECHAT_RC_OK\n"
          "}\n"
          "weechat::hook_command unload_running_cmd {} {} {} {} cmd_cb {}\n" },
#endif
#ifdef HAVE_GUILE
        { "guile", "scm",
          "(weechat:register \"unload_running\" \"me\" \"0.1\" \"GPL3\" "
          "\"\" \"\" \"\")\n"
          "(define (cmd_cb data buffer args)\n"
          "  (weechat:command \"\" \"/guile unload unload_running\")\n"
          "  (weechat:print \"\" \"unload_running: after unload\")\n"
          "  weechat:WEECHAT_RC_OK)\n"
          "(weechat:hook_command \"unload_running_cmd\" \"\" \"\" \"\" "
          "\"\" \"cmd_cb\" \"\")\n" },
#endif
#ifdef HAVE_JAVASCRIPT
        { "javascript", "js",
          "weechat.register('unload_running', 'me', '0.1', 'GPL3', '', '', '');\n"
          "function cmd_cb(data, buffer, args) {\n"
          "  weechat.command('', '/javascript unload unload_running');\n"
          "  weechat.print('', 'unload_running: after unload');\n"
          "  return weechat.WEECHAT_RC_OK;\n"
          "}\n"
          "weechat.hook_command('unload_running_cmd', '', '', '', '', "
          "'cmd_cb', '');\n" },
#endif
#ifdef HAVE_PHP
        { "php", "php",
          "<?php\n"
          "weechat_register('unload_running', 'me', '0.1', 'GPL3', '', '', '');\n"
          "function cmd_cb($data, $buffer, $args) {\n"
          "  weechat_command('', '/php unload unload_running');\n"
          "  weechat_print('', 'unload_running: after unload');\n"
          "  return WEECHAT_RC_OK;\n"
          "}\n"
          "weechat_hook_command('unload_running_cmd', '', '', '', '', "
          "'cmd_cb', '');\n" },
#endif
        { NULL, NULL, NULL }
    };
    char *path_dir, path[PATH_MAX], str_command[PATH_MAX + 128];
    int i, turnoff_memleak, loaded_after_cmd, loaded_after_timers;

    path_dir = string_eval_path_home ("${weechat_data_dir}/test_unload_running",
                                      NULL, NULL, NULL);
    CHECK(path_dir);
    dir_rmtree (path_dir);
    LONGS_EQUAL(1, dir_mkdir_parents (path_dir, 0755));

    for (i = 0; languages[i][0]; i++)
    {
        snprintf (path, sizeof (path), "%s/unload_running.%s",
                  path_dir, languages[i][1]);
        LONGS_EQUAL(1, test_scripts_write_file (path, languages[i][2]));

        /*
         * TODO: fix memory leaks in javascript plugin
         * and keep memory leak detection enabled
         */
        turnoff_memleak = (strcmp (languages[i][0], "javascript") == 0);

        if (turnoff_memleak)
            MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();

        snprintf (str_command, sizeof (str_command),
                  "/%s load %s", languages[i][0], path);
        run_cmd_quiet (str_command);
        LONGS_EQUAL(1, test_scripts_is_loaded (languages[i][0],
                                               "unload_running"));

        /*
         * the script unloads itself in its command callback: the unload is
         * delayed, then done by a timer
         */
        record_start ();
        run_cmd_quiet ("/unload_running_cmd");
        record_stop ();
        loaded_after_cmd = test_scripts_is_loaded (languages[i][0],
                                                   "unload_running");
        test_scripts_wait_unload (languages[i][0], "unload_running", 1000);
        loaded_after_timers = test_scripts_is_loaded (languages[i][0],
                                                      "unload_running");

        /*
         * enable memory leak detection again before checks:
         * a failed check exits the test immediately
         */
        if (turnoff_memleak)
            MemoryLeakWarningPlugin::turnOnThreadSafeNewDeleteOverloads();

        RECORD_CHECK_MSG("core.weechat", "", "unload_running: after unload",
                         NULL);
        LONGS_EQUAL(1, loaded_after_cmd);
        LONGS_EQUAL(0, loaded_after_timers);
    }

    dir_rmtree (path_dir);
    free (path_dir);
}
