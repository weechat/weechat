/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test script repository functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "src/plugins/script/script-repo.h"
}

TEST_GROUP(ScriptRepo)
{
};

/*
 * Test functions:
 *   script_repo_script_name_valid
 */

TEST(ScriptRepo, ScriptNameValid)
{
    /* invalid script name */
    LONGS_EQUAL(0, script_repo_script_name_valid (NULL));
    LONGS_EQUAL(0, script_repo_script_name_valid (""));

    /* directory separator */
    LONGS_EQUAL(0, script_repo_script_name_valid ("/"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("/etc/passwd"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("../test"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("../../test"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("dir/test"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("test/"));

    /* Windows directory separator */
    LONGS_EQUAL(0, script_repo_script_name_valid ("\\"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("..\\test"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("dir\\test"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("test\\"));

    /* Windows drive separator */
    LONGS_EQUAL(0, script_repo_script_name_valid (":"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("c:"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("c:test"));
    LONGS_EQUAL(0, script_repo_script_name_valid ("test:"));

    /*
     * "." and ".." are accepted: they are single path components and the
     * script extension is always appended to the name, so they can not be
     * used to escape the scripts directory
     */
    LONGS_EQUAL(1, script_repo_script_name_valid ("."));
    LONGS_EQUAL(1, script_repo_script_name_valid (".."));

    /* valid script name */
    LONGS_EQUAL(1, script_repo_script_name_valid ("a"));
    LONGS_EQUAL(1, script_repo_script_name_valid ("go"));
    LONGS_EQUAL(1, script_repo_script_name_valid ("iset"));
    LONGS_EQUAL(1, script_repo_script_name_valid (".test"));
    LONGS_EQUAL(1, script_repo_script_name_valid ("test.py"));
    LONGS_EQUAL(1, script_repo_script_name_valid ("buffer_autoset"));
    LONGS_EQUAL(1, script_repo_script_name_valid ("weechat-script"));
    LONGS_EQUAL(1, script_repo_script_name_valid ("noël"));
}
