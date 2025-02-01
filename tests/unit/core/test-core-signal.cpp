/*
 * test-core-signal.cpp - test util functions
 *
 * Copyright (C) 2021-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#include "CppUTest/TestHarness.h"

#include "tests/tests.h"

extern "C"
{
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "src/core/core-signal.h"
}

TEST_GROUP(CoreSignal)
{
};

/*
 * Tests functions:
 *   signal_search_number
 */

TEST(CoreSignal, SearchNumber)
{
    int count, index;

    /* make tests fail if the signal_list structure is changed */
    for (count = 0; signal_list[count].name; count++)
    {
    }
    LONGS_EQUAL(7, count);

    LONGS_EQUAL(-1, signal_search_number (-1));
    LONGS_EQUAL(-1, signal_search_number (999999999));

    index = signal_search_number (SIGHUP);
    LONGS_EQUAL(SIGHUP, signal_list[index].signal);
    STRCMP_EQUAL("hup", signal_list[index].name);

    index = signal_search_number (SIGINT);
    LONGS_EQUAL(SIGINT, signal_list[index].signal);
    STRCMP_EQUAL("int", signal_list[index].name);

    index = signal_search_number (SIGQUIT);
    LONGS_EQUAL(SIGQUIT, signal_list[index].signal);
    STRCMP_EQUAL("quit", signal_list[index].name);

    index = signal_search_number (SIGKILL);
    LONGS_EQUAL(SIGKILL, signal_list[index].signal);
    STRCMP_EQUAL("kill", signal_list[index].name);

    index = signal_search_number (SIGTERM);
    LONGS_EQUAL(SIGTERM, signal_list[index].signal);
    STRCMP_EQUAL("term", signal_list[index].name);

    index = signal_search_number (SIGUSR1);
    LONGS_EQUAL(SIGUSR1, signal_list[index].signal);
    STRCMP_EQUAL("usr1", signal_list[index].name);

    index = signal_search_number (SIGUSR2);
    LONGS_EQUAL(SIGUSR2, signal_list[index].signal);
    STRCMP_EQUAL("usr2", signal_list[index].name);
}

/*
 * Tests functions:
 *   signal_search_name
 */

TEST(CoreSignal, SearchName)
{
    LONGS_EQUAL(-1, signal_search_name (NULL));
    LONGS_EQUAL(-1, signal_search_name (""));
    LONGS_EQUAL(-1, signal_search_name ("signal_does_not_exist"));

    LONGS_EQUAL(SIGHUP, signal_search_name ("hup"));
    LONGS_EQUAL(SIGHUP, signal_search_name ("HUP"));
    LONGS_EQUAL(SIGINT, signal_search_name ("int"));
    LONGS_EQUAL(SIGINT, signal_search_name ("INT"));
    LONGS_EQUAL(SIGQUIT, signal_search_name ("quit"));
    LONGS_EQUAL(SIGQUIT, signal_search_name ("QUIT"));
    LONGS_EQUAL(SIGKILL, signal_search_name ("kill"));
    LONGS_EQUAL(SIGKILL, signal_search_name ("KILL"));
    LONGS_EQUAL(SIGTERM, signal_search_name ("term"));
    LONGS_EQUAL(SIGTERM, signal_search_name ("TERM"));
    LONGS_EQUAL(SIGUSR1, signal_search_name ("usr1"));
    LONGS_EQUAL(SIGUSR1, signal_search_name ("USR1"));
    LONGS_EQUAL(SIGUSR2, signal_search_name ("usr2"));
    LONGS_EQUAL(SIGUSR2, signal_search_name ("USR2"));
}

/*
 * Tests functions:
 *   signal_catch
 */

TEST(CoreSignal, Catch)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   signal_send_to_weechat
 */

TEST(CoreSignal, SentToWeechat)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   signal_exec_command
 */

TEST(CoreSignal, ExecCommand)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   signal_handle_number
 */

TEST(CoreSignal, HandleNumber)
{
    /* TODO: write tests */
}


/*
 * Tests functions:
 *   signal_handle
 */

TEST(CoreSignal, Handle)
{
    /* TODO: write tests */
}
