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
