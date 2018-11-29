/*
 * test-plugins.cpp - test plugins
 *
 * Copyright (C) 2003-2018 Sébastien Helleu <flashcode@flashtux.org>
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

extern "C"
{
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include "src/core/wee-hdata.h"
#include "src/core/wee-hook.h"
#include "src/plugins/plugin.h"
}

TEST_GROUP(Plugins)
{
};

/*
 * Tests loaded plugins.
 */

TEST(Plugins, Loaded)
{
    struct t_hdata *hdata;
    void *plugins;

    hdata = hook_hdata_get (NULL, "plugin");
    CHECK(hdata);

    plugins = hdata_get_list (hdata, "weechat_plugins");
    CHECK(plugins);

    /* check that all plugins are properly loaded */
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == alias", 1));
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == aspell", 1));
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == buflist", 1));
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == charset", 1));
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == exec", 1));
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == fifo", 1));
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == fset", 1));
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == guile", 1));
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == irc", 1));
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == javascript", 1));
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == logger", 1));
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == lua", 1));
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == perl", 1));
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == php", 1));
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == python", 1));
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == relay", 1));
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == ruby", 1));
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == script", 1));
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == tcl", 1));
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == trigger", 1));
    CHECK(hdata_search (hdata, plugins, "${plugin.name} == xfer", 1));

    /* non-existing plugin */
    POINTERS_EQUAL(NULL,
                   hdata_search (hdata, plugins, "${plugin.name} == x", 1));
}
