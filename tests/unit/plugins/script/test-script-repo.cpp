/*
 * test-script-repo.cpp - test script repository functions
 *
 * Copyright (C) 2026 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "src/plugins/script/script-repo.h"
}

TEST_GROUP(ScriptRepo)
{
};

/*
 * Tests functions:
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
