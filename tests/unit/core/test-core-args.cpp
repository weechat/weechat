/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test command-line arguments functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "src/core/core-args.h"
#include "src/core/core-list.h"
#include "src/core/core-string.h"
#include "src/core/weechat.h"
}

#define WEE_CHECK_ESCAPE(__result, __string, __length)                  \
    str = args_escape (__string, __length);                             \
    STRCMP_EQUAL(__result, str);                                        \
    free (str);

#define WEE_CHECK_INVALID_OPTION(__result, __arg)                       \
    str = args_get_invalid_option (__arg, ARGS_SHORT_OPTIONS);          \
    STRCMP_EQUAL(__result, str);                                        \
    free (str);

TEST_GROUP(CoreArgs)
{
};

/*
 * Test functions:
 *   args_escape
 */

TEST(CoreArgs, Escape)
{
    char *str;

    POINTERS_EQUAL(NULL, args_escape (NULL, 0));
    POINTERS_EQUAL(NULL, args_escape ("abc", -1));

    WEE_CHECK_ESCAPE("", "", 0);
    WEE_CHECK_ESCAPE("", "abc", 0);
    WEE_CHECK_ESCAPE("ab", "abc", 2);
    WEE_CHECK_ESCAPE("abc", "abc", 3);
    WEE_CHECK_ESCAPE("-é", "-é", strlen ("-é"));
    WEE_CHECK_ESCAPE("noël ⇒ 日本", "noël ⇒ 日本", strlen ("noël ⇒ 日本"));

    /* Control chars. */
    WEE_CHECK_ESCAPE("\\x01", "\x01", 1);
    WEE_CHECK_ESCAPE("-\\x1B[2J", "-\x1b[2J", 5);
    WEE_CHECK_ESCAPE("a\\x09b\\x0Ac\\x7F", "a\tb\nc\x7f", 6);

    /* C1 control chars (U+0080 to U+009F), not U+00A0. */
    WEE_CHECK_ESCAPE("\\xC2\\x80", "\xc2\x80", 2);
    WEE_CHECK_ESCAPE("-\\xC2\\x9B31m", "-\xc2\x9b" "31m", 6);
    WEE_CHECK_ESCAPE("\\xC2\\x9F", "\xc2\x9f", 2);
    WEE_CHECK_ESCAPE("\xc2\xa0", "\xc2\xa0", 2);

    /* Invalid UTF-8. */
    WEE_CHECK_ESCAPE("\\xFF", "\xff", 1);
    WEE_CHECK_ESCAPE("a\\xC3b", "a\xc3" "b", 3);
    /* Incomplete UTF-8 char at the end of the string. */
    WEE_CHECK_ESCAPE("\\xC3", "é", 1);
}

/*
 * Test functions:
 *   args_get_invalid_option
 */

TEST(CoreArgs, GetInvalidOption)
{
    char *str;

    POINTERS_EQUAL(NULL, args_get_invalid_option (NULL, NULL));
    POINTERS_EQUAL(NULL,
                   args_get_invalid_option (NULL, ARGS_SHORT_OPTIONS));

    /* Unknown or ambiguous long option. */
    WEE_CHECK_INVALID_OPTION("--foo", "--foo");
    WEE_CHECK_INVALID_OPTION("--foo", "--foo=bar");
    WEE_CHECK_INVALID_OPTION("--foo", "--foo=");
    WEE_CHECK_INVALID_OPTION("--", "--=bar");
    WEE_CHECK_INVALID_OPTION("--d", "--d");
    WEE_CHECK_INVALID_OPTION("--fo\\x1Bo", "--fo\x1bo");

    /* Long option given with an unexpected argument. */
    WEE_CHECK_INVALID_OPTION("--help", "--help=x");
    WEE_CHECK_INVALID_OPTION("--he", "--he=x");

    /* Long option with a missing argument. */
    WEE_CHECK_INVALID_OPTION("--dir", "--dir");

    /* Unknown short option. */
    WEE_CHECK_INVALID_OPTION("-Z", "-Z");
    WEE_CHECK_INVALID_OPTION("-Z", "-aZ");
    WEE_CHECK_INVALID_OPTION("-Z", "-aZs");
    WEE_CHECK_INVALID_OPTION("-Z", "-asZY");
    WEE_CHECK_INVALID_OPTION("-:", "-:");
    WEE_CHECK_INVALID_OPTION("-a-", "-a-");
    WEE_CHECK_INVALID_OPTION("--", "--");
    WEE_CHECK_INVALID_OPTION("- ", "- ");

    /* Unknown short option: not an ASCII char. */
    WEE_CHECK_INVALID_OPTION("-é", "-é");
    WEE_CHECK_INVALID_OPTION("-é", "-aé");
    WEE_CHECK_INVALID_OPTION("-日", "-as日本");

    /* Unknown short option: control char or invalid UTF-8. */
    WEE_CHECK_INVALID_OPTION("-\\xC2\\x9B", "-a\xc2\x9b" "31m");
    WEE_CHECK_INVALID_OPTION("--\\xC2\\x9B2J", "--\xc2\x9b" "2J");
    WEE_CHECK_INVALID_OPTION("-\\x01", "-\x01");
    WEE_CHECK_INVALID_OPTION("-\\x1B", "-a\x1b[2J");
    WEE_CHECK_INVALID_OPTION("-\\xFF", "-a\xff");
    WEE_CHECK_INVALID_OPTION("-\\xC3", "-\xc3" "a");

    /* Short option with a missing argument. */
    WEE_CHECK_INVALID_OPTION("-d", "-d");
    WEE_CHECK_INVALID_OPTION("-d", "-pd");
    WEE_CHECK_INVALID_OPTION("-r", "-asr");

    /* Short option with its argument in the same group: no error found. */
    WEE_CHECK_INVALID_OPTION("-dZ", "-dZ");

    /* Without optstring: whole argument. */
    str = args_get_invalid_option ("-aZ", NULL);
    STRCMP_EQUAL("-aZ", str);
    free (str);

    /* Not an option: whole argument. */
    WEE_CHECK_INVALID_OPTION("abc", "abc");
    WEE_CHECK_INVALID_OPTION("-", "-");
}

#define WEE_CHECK_LONG_OPTION(__result, __index, __arg)                 \
    index_option = -2;                                                  \
    LONGS_EQUAL(__result,                                               \
                args_check_long_option (__arg, long_options,            \
                                        &index_option));                \
    LONGS_EQUAL(__index, index_option);

/*
 * Test functions:
 *   args_check_long_option
 */

TEST(CoreArgs, CheckLongOption)
{
    struct option long_options[] = {
        { "help",       no_argument,       NULL, 'h' },
        { "dir",        required_argument, NULL, 'd' },
        { "no-connect", no_argument,       NULL, 'a' },
        { "no-script",  no_argument,       NULL, 's' },
        { "temp",       no_argument,       NULL, 't' },
        { "temp-dir",   no_argument,       NULL, 'T' },
        { NULL,         0,                 NULL, 0   },
    };
    int index_option;

    /* Invalid arguments. */
    LONGS_EQUAL(ARGS_OPTION_ERROR_UNKNOWN,
                args_check_long_option (NULL, long_options, NULL));
    LONGS_EQUAL(ARGS_OPTION_ERROR_UNKNOWN,
                args_check_long_option ("--help=x", NULL, NULL));
    LONGS_EQUAL(ARGS_OPTION_ERROR_ARGUMENT_NOT_ALLOWED,
                args_check_long_option ("--help=x", long_options, NULL));

    /* Not a long option. */
    WEE_CHECK_LONG_OPTION(ARGS_OPTION_ERROR_UNKNOWN, -1, "-h");
    WEE_CHECK_LONG_OPTION(ARGS_OPTION_ERROR_UNKNOWN, -1, "help");
    WEE_CHECK_LONG_OPTION(ARGS_OPTION_ERROR_UNKNOWN, -1, "--");
    WEE_CHECK_LONG_OPTION(ARGS_OPTION_ERROR_UNKNOWN, -1, "--=x");

    /* Unknown option. */
    WEE_CHECK_LONG_OPTION(ARGS_OPTION_ERROR_UNKNOWN, -1, "--foo");
    WEE_CHECK_LONG_OPTION(ARGS_OPTION_ERROR_UNKNOWN, -1, "--foo=bar");
    WEE_CHECK_LONG_OPTION(ARGS_OPTION_ERROR_UNKNOWN, -1, "--helpx");
    WEE_CHECK_LONG_OPTION(ARGS_OPTION_ERROR_UNKNOWN, -1, "--no-x");

    /* Ambiguous option. */
    WEE_CHECK_LONG_OPTION(ARGS_OPTION_ERROR_AMBIGUOUS, -1, "--no");
    WEE_CHECK_LONG_OPTION(ARGS_OPTION_ERROR_AMBIGUOUS, -1, "--no-");
    WEE_CHECK_LONG_OPTION(ARGS_OPTION_ERROR_AMBIGUOUS, -1, "--no=x");
    WEE_CHECK_LONG_OPTION(ARGS_OPTION_ERROR_AMBIGUOUS, -1, "--te");

    /* Argument given to an option without argument. */
    WEE_CHECK_LONG_OPTION(ARGS_OPTION_ERROR_ARGUMENT_NOT_ALLOWED, 0,
                          "--help=x");
    WEE_CHECK_LONG_OPTION(ARGS_OPTION_ERROR_ARGUMENT_NOT_ALLOWED, 0,
                          "--he=");
    WEE_CHECK_LONG_OPTION(ARGS_OPTION_ERROR_ARGUMENT_NOT_ALLOWED, 2,
                          "--no-c=1");
    /* Exact name "temp" is used, even if it's a prefix of "temp-dir". */
    WEE_CHECK_LONG_OPTION(ARGS_OPTION_ERROR_ARGUMENT_NOT_ALLOWED, 4,
                          "--temp=x");
    WEE_CHECK_LONG_OPTION(ARGS_OPTION_ERROR_ARGUMENT_NOT_ALLOWED, 5,
                          "--temp-=x");

    /* Valid option: not an error for this function (found but no error). */
    WEE_CHECK_LONG_OPTION(ARGS_OPTION_ERROR_UNKNOWN, 0, "--help");
    WEE_CHECK_LONG_OPTION(ARGS_OPTION_ERROR_UNKNOWN, 1, "--dir=/tmp");
    WEE_CHECK_LONG_OPTION(ARGS_OPTION_ERROR_UNKNOWN, 4, "--temp");
}

/*
 * Search a long option by name in an array of long options.
 *
 * Return the val of option, -1 if not found.
 */

int
test_args_search_long_option (struct option *long_options, const char *name)
{
    int i;

    for (i = 0; long_options[i].name; i++)
    {
        if (strcmp (long_options[i].name, name) == 0)
            return long_options[i].val;
    }
    return -1;
}

/*
 * Test functions:
 *   args_option_is_headless_only
 *   args_get_long_options
 */

TEST(CoreArgs, GetLongOptions)
{
    struct option long_options[128];
    const char *headless_options[] = { "daemon", "stdout", "doc-gen", NULL };
    const char *common_options[] = {
        "no-connect", "colors", "dir", "temp-dir", "help", "build-info",
        "license", "no-plugin", "plugins", "run-command", "no-script",
        "upgrade", "version", "no-dlclose", "no-gnutls", "no-gcrypt", NULL,
    };
    int i, val, count_common, count_headless;

    LONGS_EQUAL(0, args_option_is_headless_only (0));
    LONGS_EQUAL(0, args_option_is_headless_only ('h'));
    LONGS_EQUAL(0, args_option_is_headless_only (-1));

    /* Must not crash. */
    args_get_long_options (NULL, 0);
    args_get_long_options (NULL, 1);

    /* Headless mode: all options. */
    memset (long_options, 0xFF, sizeof (long_options));
    args_get_long_options (long_options, 1);
    for (i = 0; common_options[i]; i++)
    {
        val = test_args_search_long_option (long_options, common_options[i]);
        CHECK(val != -1);
        LONGS_EQUAL(0, args_option_is_headless_only (val));
    }
    for (i = 0; headless_options[i]; i++)
    {
        val = test_args_search_long_option (long_options, headless_options[i]);
        CHECK(val != -1);
        LONGS_EQUAL(1, args_option_is_headless_only (val));
    }
    for (count_headless = 0; long_options[count_headless].name;
         count_headless++)
    {
    }
    LONGS_EQUAL(0, long_options[count_headless].has_arg);
    POINTERS_EQUAL(NULL, long_options[count_headless].flag);
    LONGS_EQUAL(0, long_options[count_headless].val);

    /* Other modes: options for headless mode are removed. */
    memset (long_options, 0xFF, sizeof (long_options));
    args_get_long_options (long_options, 0);
    for (i = 0; common_options[i]; i++)
    {
        CHECK(test_args_search_long_option (long_options,
                                            common_options[i]) != -1);
    }
    for (i = 0; headless_options[i]; i++)
    {
        LONGS_EQUAL(-1, test_args_search_long_option (long_options,
                                                      headless_options[i]));
    }
    for (count_common = 0; long_options[count_common].name; count_common++)
    {
    }
    LONGS_EQUAL(count_headless - 3, count_common);
    LONGS_EQUAL(0, long_options[count_common].has_arg);
    POINTERS_EQUAL(NULL, long_options[count_common].flag);
    LONGS_EQUAL(0, long_options[count_common].val);
}

TEST_GROUP(CoreArgsParse)
{
    /* Variables set by args_parse, saved before each test. */
    char *saved_argv0;
    int saved_upgrading;
    char *saved_home_force;
    int saved_home_temp;
    int saved_home_delete_on_exit;
    int saved_server_cmd_line;
    char *saved_force_plugin_autoload;
    struct t_weelist *saved_startup_commands;
    int saved_doc_gen;
    char *saved_doc_gen_path;
    int saved_plugin_no_dlclose;
    int saved_no_gnutls;
    int saved_no_gcrypt;
    int saved_daemon;
    int saved_log_stdout;
    int saved_headless;
    int saved_plugin_argc;
    char **saved_plugin_argv;
    int saved_action;

    /* Arguments given to args_parse (plugin arguments point to them). */
    char **test_argv;

    /* Plugin arguments as a string, for example: "[irc://x] [-a]". */
    char plugin_args[1024];

    /*
     * Parse command line arguments (the program name "weechat" is added
     * before them), in headless mode or not.
     *
     * The arguments must not make args_parse exit (no invalid option).
     */

    void parse (const char *args, int headless)
    {
        char *command;
        int argc, i;

        string_free_split (test_argv);
        test_argv = NULL;

        command = (char *)malloc (strlen (args) + 16);
        CHECK(command);
        snprintf (command, strlen (args) + 16, "weechat %s", args);
        test_argv = string_split_shell (command, &argc);
        free (command);
        CHECK(test_argv);

        weechat_headless = headless;
        args_parse (argc, test_argv);

        plugin_args[0] = '\0';
        CHECK(weechat_plugin_argv);
        for (i = 0; i < weechat_plugin_argc; i++)
        {
            if (i > 0)
                strcat (plugin_args, " ");
            strcat (plugin_args, "[");
            strcat (plugin_args, weechat_plugin_argv[i]);
            strcat (plugin_args, "]");
        }
        POINTERS_EQUAL(NULL, weechat_plugin_argv[weechat_plugin_argc]);
    }

    void setup ()
    {
        saved_argv0 = weechat_argv0;
        saved_upgrading = weechat_upgrading;
        saved_home_force = weechat_home_force;
        saved_home_temp = weechat_home_temp;
        saved_home_delete_on_exit = weechat_home_delete_on_exit;
        saved_server_cmd_line = weechat_server_cmd_line;
        saved_force_plugin_autoload = weechat_force_plugin_autoload;
        saved_startup_commands = weechat_startup_commands;
        saved_doc_gen = weechat_doc_gen;
        saved_doc_gen_path = weechat_doc_gen_path;
        saved_plugin_no_dlclose = weechat_plugin_no_dlclose;
        saved_no_gnutls = weechat_no_gnutls;
        saved_no_gcrypt = weechat_no_gcrypt;
        saved_daemon = weechat_daemon;
        saved_log_stdout = weechat_log_stdout;
        saved_headless = weechat_headless;
        saved_plugin_argc = weechat_plugin_argc;
        saved_plugin_argv = weechat_plugin_argv;
        saved_action = args_action;

        /* Detach the saved values, so that args_parse does not free them. */
        weechat_argv0 = NULL;
        weechat_home_force = NULL;
        weechat_force_plugin_autoload = NULL;
        weechat_startup_commands = NULL;
        weechat_doc_gen_path = NULL;
        weechat_plugin_argv = NULL;
        weechat_plugin_argc = 0;

        test_argv = NULL;
        plugin_args[0] = '\0';
    }

    void teardown ()
    {
        /* Free the values set by args_parse. */
        free (weechat_argv0);
        free (weechat_home_force);
        free (weechat_force_plugin_autoload);
        weelist_free (weechat_startup_commands);
        free (weechat_doc_gen_path);
        free (weechat_plugin_argv);
        string_free_split (test_argv);

        weechat_argv0 = saved_argv0;
        weechat_upgrading = saved_upgrading;
        weechat_home_force = saved_home_force;
        weechat_home_temp = saved_home_temp;
        weechat_home_delete_on_exit = saved_home_delete_on_exit;
        weechat_server_cmd_line = saved_server_cmd_line;
        weechat_force_plugin_autoload = saved_force_plugin_autoload;
        weechat_startup_commands = saved_startup_commands;
        weechat_doc_gen = saved_doc_gen;
        weechat_doc_gen_path = saved_doc_gen_path;
        weechat_plugin_no_dlclose = saved_plugin_no_dlclose;
        weechat_no_gnutls = saved_no_gnutls;
        weechat_no_gcrypt = saved_no_gcrypt;
        weechat_daemon = saved_daemon;
        weechat_log_stdout = saved_log_stdout;
        weechat_headless = saved_headless;
        weechat_plugin_argc = saved_plugin_argc;
        weechat_plugin_argv = saved_plugin_argv;
        args_action = saved_action;
    }
};

/*
 * Test functions:
 *   args_parse (no argument, default values)
 */

TEST(CoreArgsParse, NoArgument)
{
    parse ("", 0);
    STRCMP_EQUAL("weechat", weechat_argv0);
    LONGS_EQUAL(0, weechat_upgrading);
    POINTERS_EQUAL(NULL, weechat_home_force);
    LONGS_EQUAL(0, weechat_home_temp);
    POINTERS_EQUAL(NULL, weechat_force_plugin_autoload);
    POINTERS_EQUAL(NULL, weechat_startup_commands);
    LONGS_EQUAL(0, weechat_doc_gen);
    POINTERS_EQUAL(NULL, weechat_doc_gen_path);
    LONGS_EQUAL(0, weechat_plugin_no_dlclose);
    LONGS_EQUAL(0, weechat_no_gnutls);
    LONGS_EQUAL(0, weechat_no_gcrypt);
    LONGS_EQUAL(0, weechat_daemon);
    LONGS_EQUAL(0, weechat_log_stdout);
    LONGS_EQUAL(ARGS_ACTION_NONE, args_action);
    LONGS_EQUAL(0, weechat_plugin_argc);
    STRCMP_EQUAL("", plugin_args);
}

/*
 * Test functions:
 *   args_parse (options "-d", "-t")
 */

TEST(CoreArgsParse, HomeDir)
{
    parse ("-d /tmp/x", 0);
    STRCMP_EQUAL("/tmp/x", weechat_home_force);
    LONGS_EQUAL(0, weechat_home_temp);

    parse ("--dir=/tmp/y", 0);
    STRCMP_EQUAL("/tmp/y", weechat_home_force);

    parse ("--di /tmp/z", 0);
    STRCMP_EQUAL("/tmp/z", weechat_home_force);

    parse ("-d/tmp/x -d /tmp/y", 0);
    STRCMP_EQUAL("/tmp/y", weechat_home_force);

    parse ("-t", 0);
    POINTERS_EQUAL(NULL, weechat_home_force);
    LONGS_EQUAL(1, weechat_home_temp);

    /* The last option wins. */
    parse ("-d /tmp/x -t", 0);
    POINTERS_EQUAL(NULL, weechat_home_force);
    LONGS_EQUAL(1, weechat_home_temp);

    parse ("--temp-dir -d /tmp/x", 0);
    STRCMP_EQUAL("/tmp/x", weechat_home_force);
    LONGS_EQUAL(0, weechat_home_temp);
}

/*
 * Test functions:
 *   args_parse (options "-p", "-P", "-r", "--upgrade", debug options)
 */

TEST(CoreArgsParse, Options)
{
    parse ("-p", 0);
    STRCMP_EQUAL("!*", weechat_force_plugin_autoload);

    parse ("-P irc,relay", 0);
    STRCMP_EQUAL("irc,relay", weechat_force_plugin_autoload);

    parse ("-p -P irc", 0);
    STRCMP_EQUAL("irc", weechat_force_plugin_autoload);

    parse ("-pPirc", 0);
    STRCMP_EQUAL("irc", weechat_force_plugin_autoload);

    parse ("--plugins=irc -p", 0);
    STRCMP_EQUAL("!*", weechat_force_plugin_autoload);

    parse ("-r '/print a' --run-command=/print_b -r /c", 0);
    CHECK(weechat_startup_commands);
    LONGS_EQUAL(3, weelist_size (weechat_startup_commands));
    STRCMP_EQUAL("/print a",
                 weelist_string (weelist_get (weechat_startup_commands, 0)));
    STRCMP_EQUAL("/print_b",
                 weelist_string (weelist_get (weechat_startup_commands, 1)));
    STRCMP_EQUAL("/c",
                 weelist_string (weelist_get (weechat_startup_commands, 2)));

    parse ("--upgrade", 0);
    LONGS_EQUAL(1, weechat_upgrading);

    parse ("--no-dlclose --no-gnutls --no-gcrypt", 0);
    LONGS_EQUAL(1, weechat_plugin_no_dlclose);
    LONGS_EQUAL(1, weechat_no_gnutls);
    LONGS_EQUAL(1, weechat_no_gcrypt);
}

/*
 * Test functions:
 *   args_parse (variables reset on each call)
 */

TEST(CoreArgsParse, Reset)
{
    parse ("-d /tmp/x -P irc -r /a --upgrade --no-dlclose --no-gnutls "
           "--no-gcrypt -a irc://x", 0);
    CHECK(weechat_home_force);
    CHECK(weechat_force_plugin_autoload);
    CHECK(weechat_startup_commands);
    STRCMP_EQUAL("[irc://x] [-a]", plugin_args);

    parse ("", 0);
    POINTERS_EQUAL(NULL, weechat_home_force);
    POINTERS_EQUAL(NULL, weechat_force_plugin_autoload);
    POINTERS_EQUAL(NULL, weechat_startup_commands);
    LONGS_EQUAL(0, weechat_upgrading);
    LONGS_EQUAL(0, weechat_plugin_no_dlclose);
    LONGS_EQUAL(0, weechat_no_gnutls);
    LONGS_EQUAL(0, weechat_no_gcrypt);
    STRCMP_EQUAL("", plugin_args);

    parse ("--doc-gen /tmp/doc", 1);
    LONGS_EQUAL(1, weechat_doc_gen);
    CHECK(weechat_doc_gen_path);

    parse ("", 1);
    LONGS_EQUAL(0, weechat_doc_gen);
    POINTERS_EQUAL(NULL, weechat_doc_gen_path);

    parse ("--daemon", 1);
    LONGS_EQUAL(1, weechat_daemon);

    parse ("", 1);
    LONGS_EQUAL(0, weechat_daemon);

    parse ("--stdout", 1);
    LONGS_EQUAL(1, weechat_log_stdout);

    parse ("", 1);
    LONGS_EQUAL(0, weechat_log_stdout);
}

/*
 * Test functions:
 *   args_parse (arguments for plugins)
 */

TEST(CoreArgsParse, PluginArgs)
{
    /* Non-option arguments. */
    parse ("irc://x", 0);
    STRCMP_EQUAL("[irc://x]", plugin_args);
    parse ("irc://x irc://y", 0);
    STRCMP_EQUAL("[irc://x] [irc://y]", plugin_args);
    parse ("irc://x - irc://y", 0);
    STRCMP_EQUAL("[irc://x] [irc://y]", plugin_args);

    /* Options "-a" and "-s" (always at the end, "-a" first). */
    parse ("-a", 0);
    STRCMP_EQUAL("[-a]", plugin_args);
    parse ("-s", 0);
    STRCMP_EQUAL("[-s]", plugin_args);
    parse ("-s -a -a", 0);
    STRCMP_EQUAL("[-a] [-s]", plugin_args);
    parse ("-as", 0);
    STRCMP_EQUAL("[-a] [-s]", plugin_args);
    parse ("-sa", 0);
    STRCMP_EQUAL("[-a] [-s]", plugin_args);
    parse ("--no-connect --no-script", 0);
    STRCMP_EQUAL("[-a] [-s]", plugin_args);
    parse ("--no-conn --no-sc", 0);
    STRCMP_EQUAL("[-a] [-s]", plugin_args);
    parse ("-pa", 0);
    STRCMP_EQUAL("[-a]", plugin_args);
    parse ("-a irc://x -s irc://y", 0);
    STRCMP_EQUAL("[irc://x] [irc://y] [-a] [-s]", plugin_args);

    /* Program name and values of options are not sent to plugins. */
    parse ("-d /tmp/x -r irc_cmd -P irc irc://y", 0);
    STRCMP_EQUAL("[irc://y]", plugin_args);
    parse ("-dirc_dir -rirc_cmd", 0);
    STRCMP_EQUAL("", plugin_args);

    /* Arguments after "--" (except those starting with "-"). */
    parse ("-- irc://x", 0);
    STRCMP_EQUAL("[irc://x]", plugin_args);
    parse ("irc://x -- irc://y -z --foo - -a irc://z", 0);
    STRCMP_EQUAL("[irc://x] [irc://y] [irc://z]", plugin_args);
    parse ("-a -- -s", 0);
    STRCMP_EQUAL("[-a]", plugin_args);
    parse ("--", 0);
    STRCMP_EQUAL("", plugin_args);
}

/*
 * Test functions:
 *   args_parse (options that display something and exit)
 */

TEST(CoreArgsParse, Actions)
{
    parse ("-c", 0);
    LONGS_EQUAL(ARGS_ACTION_COLORS, args_action);
    parse ("--help", 0);
    LONGS_EQUAL(ARGS_ACTION_HELP, args_action);
    parse ("-i", 0);
    LONGS_EQUAL(ARGS_ACTION_BUILD_INFO, args_action);
    parse ("--license", 0);
    LONGS_EQUAL(ARGS_ACTION_LICENSE, args_action);
    parse ("-v", 0);
    LONGS_EQUAL(ARGS_ACTION_VERSION, args_action);
    parse ("--vers", 0);
    LONGS_EQUAL(ARGS_ACTION_VERSION, args_action);

    /* Parsing stops at the first one: next arguments are not parsed. */
    parse ("-v -h", 0);
    LONGS_EQUAL(ARGS_ACTION_VERSION, args_action);
    parse ("-hv", 0);
    LONGS_EQUAL(ARGS_ACTION_HELP, args_action);
    parse ("-v --foo -Z --dir", 0);
    LONGS_EQUAL(ARGS_ACTION_VERSION, args_action);
    parse ("-a irc://x -v -d /tmp/x irc://y", 0);
    LONGS_EQUAL(ARGS_ACTION_VERSION, args_action);
    POINTERS_EQUAL(NULL, weechat_home_force);
    STRCMP_EQUAL("[irc://x] [-a]", plugin_args);
}

/*
 * Test functions:
 *   args_parse (options for headless mode)
 */

TEST(CoreArgsParse, Headless)
{
    parse ("--daemon", 1);
    LONGS_EQUAL(1, weechat_daemon);
    LONGS_EQUAL(0, weechat_log_stdout);

    parse ("--stdout", 1);
    LONGS_EQUAL(0, weechat_daemon);
    LONGS_EQUAL(1, weechat_log_stdout);

    /* Option "--stdout" is ignored if "--daemon" is given. */
    parse ("--dae --std", 1);
    LONGS_EQUAL(1, weechat_daemon);
    LONGS_EQUAL(0, weechat_log_stdout);
    parse ("--stdout --daemon", 1);
    LONGS_EQUAL(1, weechat_daemon);
    LONGS_EQUAL(0, weechat_log_stdout);

    parse ("--doc-gen /tmp/doc", 1);
    LONGS_EQUAL(1, weechat_doc_gen);
    STRCMP_EQUAL("/tmp/doc", weechat_doc_gen_path);

    parse ("--doc-gen=/tmp/doc2 --doc-gen /tmp/doc3", 1);
    STRCMP_EQUAL("/tmp/doc3", weechat_doc_gen_path);

    /* Option "--daemon" is ignored if "--doc-gen" is given. */
    parse ("--daemon --stdout --doc-gen /tmp/doc", 1);
    LONGS_EQUAL(1, weechat_doc_gen);
    LONGS_EQUAL(0, weechat_daemon);
    LONGS_EQUAL(1, weechat_log_stdout);

    /* "--d" is "--dir" in other modes (no headless options). */
    parse ("--d /tmp/x", 0);
    STRCMP_EQUAL("/tmp/x", weechat_home_force);
}
