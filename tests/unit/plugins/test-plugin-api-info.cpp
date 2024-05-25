/*
 * test-plugin-api-info.cpp - tests API info functions
 *
 * Copyright (C) 2023-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H
#endif
#include <unistd.h>
#include <string.h>
#include "src/core/weechat.h"
#include "src/core/core-config.h"
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/core/core-infolist.h"
#include "src/core/core-input.h"
#include "src/core/core-proxy.h"
#include "src/core/core-secure.h"
#include "src/core/core-util.h"
#include "src/core/core-version.h"
#include "src/gui/gui-bar.h"
#include "src/gui/gui-bar-item.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-color.h"
#include "src/gui/gui-filter.h"
#include "src/gui/gui-input.h"
#include "src/gui/gui-layout.h"
#include "src/gui/gui-window.h"

extern char *plugin_api_info_absolute_path (const char *directory);
}

TEST_GROUP(PluginApiInfo)
{
};

/*
 * Tests functions:
 *   plugin_api_info_version_cb
 */

TEST(PluginApiInfo, VersionCb)
{
    const char *version;
    char *str;

    version = version_get_version ();
    WEE_TEST_STR(version, hook_info_get (NULL, "version", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_version_number_cb
 */

TEST(PluginApiInfo, VersionNumberCb)
{
    char *str, str_version_number[128];

    snprintf (str_version_number, sizeof (str_version_number),
              "%d", util_version_number (version_get_version ()));
    WEE_TEST_STR(str_version_number, hook_info_get (NULL, "version_number", NULL));
    WEE_TEST_STR(str_version_number, hook_info_get (NULL, "version_number", ""));

    WEE_TEST_STR("256"        /* 0x00000100 */, hook_info_get (NULL, "version_number", "0.0.1"));
    WEE_TEST_STR("16909056"   /* 0x01020300 */, hook_info_get (NULL, "version_number", "1.2.3"));
    WEE_TEST_STR("1484470272" /* 0x587B3800 */, hook_info_get (NULL, "version_number", "88.123.56"));
}

/*
 * Tests functions:
 *   plugin_api_info_version_git_cb
 */

TEST(PluginApiInfo, VersionGitCb)
{
    const char *version_git;
    char *str;

    version_git = version_get_git ();
    WEE_TEST_STR(version_git, hook_info_get (NULL, "version_git", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_date_cb
 */

TEST(PluginApiInfo, DateCb)
{
    const char *compilation_date;
    char *str;

    compilation_date = version_get_compilation_date_time ();
    WEE_TEST_STR(compilation_date, hook_info_get (NULL, "date", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_pid_cb
 */

TEST(PluginApiInfo, PidCb)
{
    char *str, str_pid[64];

    snprintf (str_pid, sizeof (str_pid), "%d", (int)getpid ());
    WEE_TEST_STR(str_pid, hook_info_get (NULL, "pid", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_dir_separator_cb
 */

TEST(PluginApiInfo, DirSeparatorCb)
{
    char *str;

    WEE_TEST_STR(DIR_SEPARATOR, hook_info_get (NULL, "dir_separator", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_absolute_path
 */

TEST(PluginApiInfo, AbsolutePath)
{
    char *str;

    POINTERS_EQUAL(NULL, plugin_api_info_absolute_path (NULL));
    POINTERS_EQUAL(NULL, plugin_api_info_absolute_path (""));
    POINTERS_EQUAL(NULL, plugin_api_info_absolute_path ("/invalid/dir"));

    WEE_TEST_STR("/", plugin_api_info_absolute_path ("/tmp/.."));
}

/*
 * Tests functions:
 *   plugin_api_info_absolute_path
 *   plugin_api_info_weechat_config_dir_cb
 *   plugin_api_info_weechat_data_dir_cb
 *   plugin_api_info_weechat_state_dir_cb
 *   plugin_api_info_weechat_cache_dir_cb
 *   plugin_api_info_weechat_runtime_dir_cb
 */

TEST(PluginApiInfo, WeechatDir)
{
    char *str;

    str = hook_info_get (NULL, "weechat_config_dir", NULL);
    CHECK(str);
    CHECK(str[0] == '/');
    CHECK(strlen (str) > 1);
    CHECK(strstr (str, "/tmp_weechat_test"));
    free (str);

    str = hook_info_get (NULL, "weechat_data_dir", NULL);
    CHECK(str);
    CHECK(str[0] == '/');
    CHECK(strlen (str) > 1);
    CHECK(strstr (str, "/tmp_weechat_test"));
    free (str);

    str = hook_info_get (NULL, "weechat_state_dir", NULL);
    CHECK(str);
    CHECK(str[0] == '/');
    CHECK(strlen (str) > 1);
    CHECK(strstr (str, "/tmp_weechat_test"));
    free (str);

    str = hook_info_get (NULL, "weechat_cache_dir", NULL);
    CHECK(str);
    CHECK(str[0] == '/');
    CHECK(strlen (str) > 1);
    CHECK(strstr (str, "/tmp_weechat_test"));
    free (str);

    str = hook_info_get (NULL, "weechat_runtime_dir", NULL);
    CHECK(str);
    CHECK(str[0] == '/');
    CHECK(strlen (str) > 1);
    CHECK(strstr (str, "/tmp_weechat_test"));
    free (str);
}

/*
 * Tests functions:
 *   plugin_api_info_weechat_libdir_cb
 */

TEST(PluginApiInfo, WeechatLibdirCb)
{
    char *str;

    WEE_TEST_STR(WEECHAT_LIBDIR, hook_info_get (NULL, "weechat_libdir", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_weechat_sharedir_cb
 */

TEST(PluginApiInfo, WeechatSharedirCb)
{
    char *str;

    WEE_TEST_STR(WEECHAT_SHAREDIR,
                 hook_info_get (NULL, "weechat_sharedir", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_weechat_localedir_cb
 */

TEST(PluginApiInfo, WeechatLocaledirCb)
{
    char *str;

    WEE_TEST_STR(LOCALEDIR, hook_info_get (NULL, "weechat_localedir", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_weechat_site_cb
 */

TEST(PluginApiInfo, WeechatSiteCb)
{
    char *str;

    WEE_TEST_STR(WEECHAT_WEBSITE, hook_info_get (NULL, "weechat_site", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_weechat_site_download_cb
 */

TEST(PluginApiInfo, WeechatSiteDownloadCb)
{
    char *str;

    WEE_TEST_STR(WEECHAT_WEBSITE_DOWNLOAD,
                 hook_info_get (NULL, "weechat_site_download", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_weechat_upgrading_cb
 */

TEST(PluginApiInfo, WeechatUpgradingCb)
{
    char *str;

    WEE_TEST_STR("0", hook_info_get (NULL, "weechat_upgrading", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_weechat_headless_cb
 */

TEST(PluginApiInfo, WeechatHeadlessCb)
{
    char *str;

    WEE_TEST_STR("0", hook_info_get (NULL, "weechat_headless", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_weechat_daemon_cb
 */

TEST(PluginApiInfo, WeechatDaemonCb)
{
    char *str;

    WEE_TEST_STR("0", hook_info_get (NULL, "weechat_daemon", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_auto_connect_cb
 */

TEST(PluginApiInfo, AutoConnectCb)
{
    char *str;

    WEE_TEST_STR("1", hook_info_get (NULL, "auto_connect", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_auto_load_scripts_cb
 */

TEST(PluginApiInfo, AutoLoadScriptsCb)
{
    char *str;

    WEE_TEST_STR("1", hook_info_get (NULL, "auto_load_scripts", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_buffer_cb
 */

TEST(PluginApiInfo, BufferCb)
{
    char *str, str_buffer[64];

    POINTERS_EQUAL(NULL, hook_info_get (NULL, "buffer", NULL));
    POINTERS_EQUAL(NULL, hook_info_get (NULL, "buffer", ""));

    POINTERS_EQUAL(NULL, hook_info_get (NULL, "buffer", "zzz"));

    snprintf (str_buffer, sizeof (str_buffer), "%p", gui_buffers);
    WEE_TEST_STR(str_buffer, hook_info_get (NULL, "buffer", "core.weechat"));
}

/*
 * Tests functions:
 *   plugin_api_info_charset_terminal_cb
 */

TEST(PluginApiInfo, CharsetTerminalCb)
{
    char *str;

    WEE_TEST_STR (weechat_local_charset,
                  hook_info_get (NULL, "charset_terminal", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_charset_internal_cb
 */

TEST(PluginApiInfo, CharsetInternalCb)
{
    char *str;

    WEE_TEST_STR (WEECHAT_INTERNAL_CHARSET,
                  hook_info_get (NULL, "charset_internal", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_locale_cb
 */

TEST(PluginApiInfo, LocaleCb)
{
    char *str;

    WEE_TEST_STR (setlocale (LC_MESSAGES, NULL),
                  hook_info_get (NULL, "locale", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_inactivity_cb
 */

TEST(PluginApiInfo, InactivityCb)
{
    char *str, *error;
    long long inactivity;

    str = hook_info_get (NULL, "inactivity", NULL);
    inactivity = strtoll (str, &error, 10);
    CHECK(error && !error[0]);
    CHECK(inactivity >= 0);
    free (str);
}

/*
 * Tests functions:
 *   plugin_api_info_filters_enabled_cb
 */

TEST(PluginApiInfo, FiltersEnabledCb)
{
    char *str;

    WEE_TEST_STR("1", hook_info_get (NULL, "filters_enabled", NULL));

    gui_filter_global_disable ();
    WEE_TEST_STR("0", hook_info_get (NULL, "filters_enabled", NULL));

    gui_filter_global_enable ();
    WEE_TEST_STR("1", hook_info_get (NULL, "filters_enabled", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_cursor_mode_cb
 */

TEST(PluginApiInfo, CursorModeCb)
{
    char *str;

    WEE_TEST_STR("0", hook_info_get (NULL, "cursor_mode", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_mouse_cb
 */

TEST(PluginApiInfo, MouseCb)
{
    char *str;

    WEE_TEST_STR("0", hook_info_get (NULL, "mouse", NULL));

    config_file_option_set (config_look_mouse, "1", 1);
    WEE_TEST_STR("1", hook_info_get (NULL, "mouse", NULL));

    config_file_option_set (config_look_mouse, "0", 1);
    WEE_TEST_STR("0", hook_info_get (NULL, "mouse", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_term_width_cb
 */

TEST(PluginApiInfo, TermWidthCb)
{
    char *str, str_width[64];

    snprintf (str_width, sizeof (str_width), "%d", gui_window_get_width ());
    WEE_TEST_STR(str_width, hook_info_get (NULL, "term_width", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_term_height_cb
 */

TEST(PluginApiInfo, TermHeightCb)
{
    char *str, str_height[64];

    snprintf (str_height, sizeof (str_height), "%d", gui_window_get_height ());
    WEE_TEST_STR(str_height, hook_info_get (NULL, "term_height", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_term_colors_cb
 */

TEST(PluginApiInfo, TermColorsCb)
{
    char *str, str_colors[64];

    snprintf (str_colors, sizeof (str_colors),
              "%d", gui_color_get_term_colors ());
    WEE_TEST_STR(str_colors, hook_info_get (NULL, "term_colors", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_term_color_pairs_cb
 */

TEST(PluginApiInfo, TermColorPairsCb)
{
    char *str, str_color_pairs[64];

    snprintf (str_color_pairs, sizeof (str_color_pairs),
              "%d", gui_color_get_term_color_pairs ());
    WEE_TEST_STR(str_color_pairs, hook_info_get (NULL, "term_color_pairs", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_color_ansi_regex_cb
 */

TEST(PluginApiInfo, ColorAnsiRegexCb)
{
    char *str;

    WEE_TEST_STR(GUI_COLOR_REGEX_ANSI_DECODE,
                 hook_info_get (NULL, "color_ansi_regex", NULL));
}

/*
 * Tests functions:
 *   plugin_api_info_color_term2rgb_cb
 */

TEST(PluginApiInfo, ColorTerm2rgbCb)
{
    char *str;

    POINTERS_EQUAL(NULL, hook_info_get (NULL, "color_term2rgb", NULL));
    POINTERS_EQUAL(NULL, hook_info_get (NULL, "color_term2rgb", ""));

    WEE_TEST_STR("8421504", hook_info_get (NULL, "color_term2rgb", "8"));
    WEE_TEST_STR("11534080", hook_info_get (NULL, "color_term2rgb", "154"));
}

/*
 * Tests functions:
 *   plugin_api_info_color_rgb2term_cb
 */

TEST(PluginApiInfo, ColorRgb2termCb)
{
    char *str;

    POINTERS_EQUAL(NULL, hook_info_get (NULL, "color_rgb2term", NULL));
    POINTERS_EQUAL(NULL, hook_info_get (NULL, "color_rgb2term", ""));

    WEE_TEST_STR("8", hook_info_get (NULL, "color_rgb2term", "8421504"));
    WEE_TEST_STR("154", hook_info_get (NULL, "color_rgb2term", "11534080"));
    WEE_TEST_STR("11", hook_info_get (NULL, "color_rgb2term", "11534080,64"));
}

/*
 * Tests functions:
 *   plugin_api_info_nick_color_cb
 *   plugin_api_info_nick_color_name_cb
 *   plugin_api_info_nick_color_ignore_case_cb
 *   plugin_api_info_nick_color_name_ignore_case_cb
 */

TEST(PluginApiInfo, NickColor)
{
    char *str, str_color[64];

    WEE_TEST_STR("186", hook_info_get (NULL, "nick_color_name", "Nick"));
    snprintf (str_color, sizeof (str_color), "%s", gui_color_get_custom ("186"));
    WEE_TEST_STR(str_color, hook_info_get (NULL, "nick_color", "Nick"));

    WEE_TEST_STR("blue",
                 hook_info_get (NULL,
                                "nick_color_name",
                                "Nick;green,blue,red,yellow,cyan,magenta"));
    snprintf (str_color, sizeof (str_color), "%s", gui_color_get_custom ("blue"));
    WEE_TEST_STR(str_color,
                 hook_info_get (NULL,
                                "nick_color",
                                "Nick;green,blue,red,yellow,cyan,magenta"));

    WEE_TEST_STR("212",
                 hook_info_get (NULL,
                                "nick_color_name_ignore_case", "Nick;26"));
    snprintf (str_color, sizeof (str_color), "%s", gui_color_get_custom ("212"));
    WEE_TEST_STR(str_color, hook_info_get (NULL, "nick_color_ignore_case", "Nick;26"));

    WEE_TEST_STR("green",
                 hook_info_get (NULL,
                                "nick_color_name_ignore_case",
                                "Nick;26;green,blue,red,yellow,cyan,magenta"));
    snprintf (str_color, sizeof (str_color), "%s", gui_color_get_custom ("green"));
    WEE_TEST_STR(str_color,
                 hook_info_get (NULL,
                                "nick_color_ignore_case",
                                "Nick;26;green,blue,red,yellow,cyan,magenta"));
}

/*
 * Tests functions:
 *   plugin_api_info_build_uptime
 *   plugin_api_info_uptime_cb
 *   plugin_api_info_uptime_current_cb
 */

TEST(PluginApiInfo, Uptime)
{
    char *str, *error;
    long long seconds;

    POINTERS_EQUAL(NULL, hook_info_get (NULL, "uptime", "invalid"));

    str = hook_info_get (NULL, "uptime", NULL);
    STRNCMP_EQUAL("0:00:00:", str, 8);
    free (str);
    str = hook_info_get (NULL, "uptime_current", NULL);
    STRNCMP_EQUAL("0:00:00:", str, 8);
    free (str);

    WEE_TEST_STR("0", hook_info_get (NULL, "uptime", "days"));
    WEE_TEST_STR("0", hook_info_get (NULL, "uptime_current", "days"));

    str = hook_info_get (NULL, "uptime", "seconds");
    seconds = strtoll (str, &error, 10);
    CHECK(error && !error[0]);
    CHECK(seconds >= 0);
    str = hook_info_get (NULL, "uptime_current", "seconds");
    seconds = strtoll (str, &error, 10);
    CHECK(error && !error[0]);
    CHECK(seconds >= 0);
}

/*
 * Tests functions:
 *   plugin_api_info_totp_generate_cb
 *   plugin_api_info_totp_validate_cb
 */

TEST(PluginApiInfo, TotpGenerateCb)
{
    char *str;

    POINTERS_EQUAL(NULL, hook_info_get (NULL, "totp_generate", NULL));
    POINTERS_EQUAL(NULL, hook_info_get (NULL, "totp_generate", ""));

    POINTERS_EQUAL(NULL, hook_info_get (NULL, "totp_validate", NULL));
    POINTERS_EQUAL(NULL, hook_info_get (NULL, "totp_validate", ""));

    POINTERS_EQUAL(NULL, hook_info_get (NULL, "totp_generate",
                                        "secretpasswordbase32,abc"));
    POINTERS_EQUAL(NULL, hook_info_get (NULL, "totp_generate",
                                        "secretpasswordbase32,1540624066,abc"));

    POINTERS_EQUAL(NULL, hook_info_get (NULL, "totp_validate",
                                        "secretpasswordbase32,123456,abc"));
    POINTERS_EQUAL(NULL, hook_info_get (NULL, "totp_validate",
                                        "secretpasswordbase32,123456,1540624066,abc"));

    WEE_TEST_STR("065486",
                 hook_info_get (NULL,
                                "totp_generate",
                                "secretpasswordbase32,1540624066,6"));

    WEE_TEST_STR("1",
                 hook_info_get (NULL,
                                "totp_validate",
                                "secretpasswordbase32,065486,1540624066,30"));
    WEE_TEST_STR("0",
                 hook_info_get (NULL,
                                "totp_validate",
                                "secretpasswordbase32,123456,1540624066,30"));
}

/*
 * Tests functions:
 *   plugin_api_info_plugin_loaded_cb
 */

TEST(PluginApiInfo, PluginLoadedCb)
{
    char *str;

    POINTERS_EQUAL(NULL, hook_info_get (NULL, "plugin_loaded", NULL));
    POINTERS_EQUAL(NULL, hook_info_get (NULL, "plugin_loaded", ""));

    POINTERS_EQUAL(NULL, hook_info_get (NULL, "plugin_loaded", "xxx"));

    WEE_TEST_STR("1", hook_info_get (NULL, "plugin_loaded", "alias"));
    WEE_TEST_STR("1", hook_info_get (NULL, "plugin_loaded", "irc"));
}

/*
 * Tests functions:
 *   plugin_api_info_hashtable_secured_data_cb
 */

TEST(PluginApiInfo, HashtableSecuredDataCb)
{
    struct t_hashtable *hashtable;

    hashtable = hook_info_get_hashtable (NULL, "secured_data", NULL);
    CHECK(hashtable);
    LONGS_EQUAL(0, hashtable_get_integer (hashtable, "items_count"));
    hashtable_free (hashtable);

    hashtable_set (secure_hashtable_data, "password", "S3cr3t!");
    hashtable = hook_info_get_hashtable (NULL, "secured_data", NULL);
    CHECK(hashtable);
    LONGS_EQUAL(1, hashtable_get_integer (hashtable, "items_count"));
    STRCMP_EQUAL("S3cr3t!", (const char *)hashtable_get (hashtable, "password"));
    hashtable_free (hashtable);
    hashtable_remove (secure_hashtable_data, "password");
}

/*
 * Tests functions:
 *   plugin_api_infolist_bar_cb
 */

TEST(PluginApiInfo, InfolistBarCb)
{
    struct t_infolist *infolist;

    /* invalid bar pointer */
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "bar", (void *)0x1, NULL));

    /* all bars */
    infolist = hook_infolist_get (NULL, "bar", NULL, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("input", infolist_string (infolist, "name"));
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("status", infolist_string (infolist, "name"));
    CHECK(infolist_next (infolist));
    infolist_free (infolist);

    /* one bar with pointer */
    infolist = hook_infolist_get (NULL, "bar", gui_bars, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("input", infolist_string (infolist, "name"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);

    /* one bar with name (mask) */
    infolist = hook_infolist_get (NULL, "bar", NULL, "titl*");
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("title", infolist_string (infolist, "name"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);
}

/*
 * Tests functions:
 *   plugin_api_infolist_bar_item_cb
 */

TEST(PluginApiInfo, InfolistBarItemCb)
{
    struct t_infolist *infolist;

    /* invalid bar item pointer */
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "bar_item", (void *)0x1, NULL));

    /* all bar items */
    infolist = hook_infolist_get (NULL, "bar_item", NULL, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("input_paste", infolist_string (infolist, "name"));
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("input_prompt", infolist_string (infolist, "name"));
    CHECK(infolist_next (infolist));
    infolist_free (infolist);

    /* one bar item with pointer */
    infolist = hook_infolist_get (NULL, "bar_item", gui_bar_items, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("input_paste", infolist_string (infolist, "name"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);

    /* one bar item with name (mask) */
    infolist = hook_infolist_get (NULL, "bar_item", NULL, "tim*");
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("time", infolist_string (infolist, "name"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);
}

/*
 * Tests functions:
 *   plugin_api_infolist_bar_window_cb
 */

TEST(PluginApiInfo, InfolistBarWindowCb)
{
    struct t_infolist *infolist;
    struct t_gui_bar *ptr_bar;

    /* invalid bar window pointer */
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "bar_window", (void *)0x1, NULL));

    /* all bar windows */
    infolist = hook_infolist_get (NULL, "bar_window", NULL, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    ptr_bar = (struct t_gui_bar *)infolist_pointer (infolist, "bar");
    CHECK(ptr_bar);
    STRCMP_EQUAL("buflist", ptr_bar->name);
    CHECK(infolist_next (infolist));
    ptr_bar = (struct t_gui_bar *)infolist_pointer (infolist, "bar");
    CHECK(ptr_bar);
    STRCMP_EQUAL("input", ptr_bar->name);
    CHECK(infolist_next (infolist));
    infolist_free (infolist);

    /* one bar window with pointer */
    infolist = hook_infolist_get (NULL, "bar_window", gui_windows->bar_windows, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    ptr_bar = (struct t_gui_bar *)infolist_pointer (infolist, "bar");
    CHECK(ptr_bar);
    STRCMP_EQUAL("input", ptr_bar->name);
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);
}

/*
 * Tests functions:
 *   plugin_api_infolist_buffer_cb
 */

TEST(PluginApiInfo, InfolistBufferCb)
{
    struct t_infolist *infolist;

    /* invalid buffer pointer */
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "buffer", (void *)0x1, NULL));

    /* all buffers */
    infolist = hook_infolist_get (NULL, "buffer", NULL, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("core.weechat", infolist_string (infolist, "full_name"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);

    /* one buffer with pointer */
    infolist = hook_infolist_get (NULL, "buffer", gui_buffers, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("core.weechat", infolist_string (infolist, "full_name"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);

    /* one buffer with name (mask) */
    infolist = hook_infolist_get (NULL, "buffer", NULL, "core.w*");
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("core.weechat", infolist_string (infolist, "full_name"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);
}

/*
 * Tests functions:
 *   plugin_api_infolist_buffer_lines_cb
 */

TEST(PluginApiInfo, InfolistBufferLinesCb)
{
    struct t_infolist *infolist;
    time_t date;

    /* invalid buffer lines pointer */
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "buffer_lines", (void *)0x1, NULL));

    /* lines of core buffer */
    infolist = hook_infolist_get (NULL, "buffer_lines", NULL, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    date = infolist_time (infolist, "date");
    CHECK(date > 0);
    CHECK(infolist_next (infolist));
    infolist_free (infolist);

    /* lines of core buffer (using buffer pointer) */
    infolist = hook_infolist_get (NULL, "buffer_lines", gui_buffers, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    LONGS_EQUAL(date, infolist_time (infolist, "date"));
    CHECK(infolist_next (infolist));
    infolist_free (infolist);
}

/*
 * Tests functions:
 *   plugin_api_infolist_filter_cb
 */

TEST(PluginApiInfo, InfolistFilterCb)
{
    struct t_infolist *infolist;
    struct t_gui_filter *ptr_filter1, *ptr_filter2;

    /* without filters */
    infolist = hook_infolist_get (NULL, "filter", NULL, NULL);
    CHECK(infolist);
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);

    /* with two filters */
    ptr_filter1 = gui_filter_new (1, "test_filter1", "core.weechat",
                                  "tag1", "regex1.*");
    CHECK(ptr_filter1);
    ptr_filter2 = gui_filter_new (1, "test_filter2", "core.weechat",
                                  "tag2", "regex2.*");
    CHECK(ptr_filter2);
    infolist = hook_infolist_get (NULL, "filter", NULL, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("test_filter1", infolist_string (infolist, "name"));
    STRCMP_EQUAL("tag1", infolist_string (infolist, "tags"));
    LONGS_EQUAL(1, infolist_integer (infolist, "tags_count"));
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("test_filter2", infolist_string (infolist, "name"));
    STRCMP_EQUAL("tag2", infolist_string (infolist, "tags"));
    LONGS_EQUAL(1, infolist_integer (infolist, "tags_count"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);

    /* one filter with name (mask) */
    infolist = hook_infolist_get (NULL, "filter", NULL, "test_*2");
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("test_filter2", infolist_string (infolist, "name"));
    STRCMP_EQUAL("tag2", infolist_string (infolist, "tags"));
    LONGS_EQUAL(1, infolist_integer (infolist, "tags_count"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);

    gui_filter_free (ptr_filter1);
    gui_filter_free (ptr_filter2);
}

/*
 * Tests functions:
 *   plugin_api_infolist_history_cb
 */

TEST(PluginApiInfo, InfolistHistoryCb)
{
    struct t_infolist *infolist;

    /* invalid history pointer */
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "history", (void *)0x1, NULL));

    gui_input_insert_string (gui_buffers, "abc");
    gui_input_return (gui_buffers);

    /* global history */
    infolist = hook_infolist_get (NULL, "history", NULL, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("abc", infolist_string (infolist, "text"));
    infolist_free (infolist);

    /* history of core buffer */
    input_data (gui_buffers, "abc", NULL, 1, 0);
    infolist = hook_infolist_get (NULL, "history", gui_buffers, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("abc", infolist_string (infolist, "text"));
    infolist_free (infolist);
}

/*
 * Tests functions:
 *   plugin_api_infolist_hook_cb
 */

TEST(PluginApiInfo, InfolistHookCb)
{
    struct t_infolist *infolist;
    const char *ptr_name;
    struct t_hook *ptr_hook;
    char *name, str_args[1024];

    /* invalid hook pointer */
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "hook", (void *)0x1, NULL));

    /* all hooks */
    infolist = hook_infolist_get (NULL, "hook", NULL, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    ptr_hook = (struct t_hook *)infolist_pointer (infolist, "pointer");
    ptr_name = infolist_string (infolist, "command");
    CHECK(ptr_name);
    name = strdup (ptr_name);
    STRCMP_EQUAL("command", infolist_string (infolist, "type"));
    CHECK(infolist_next (infolist));
    infolist_free (infolist);

    /* one command with pointer */
    infolist = hook_infolist_get (NULL, "hook", ptr_hook, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("command", infolist_string (infolist, "type"));
    STRCMP_EQUAL(name, infolist_string (infolist, "command"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);

    /* one command with name */
    snprintf (str_args, sizeof (str_args), "command,%s", name);
    infolist = hook_infolist_get (NULL, "hook", NULL, str_args);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("command", infolist_string (infolist, "type"));
    STRCMP_EQUAL(name, infolist_string (infolist, "command"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);

    /* all completion hooks */
    infolist = hook_infolist_get (NULL, "hook", NULL, "completion");
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("completion", infolist_string (infolist, "type"));
    CHECK(infolist_next (infolist));
    infolist_free (infolist);

    free (name);
}

/*
 * Tests functions:
 *   plugin_api_infolist_hotlist_cb
 */

TEST(PluginApiInfo, InfolistHotlistCb)
{
    struct t_infolist *infolist;

    gui_buffer_set (gui_buffers, "hotlist", "2");

    /* hotlist (one buffer) */
    infolist = hook_infolist_get (NULL, "hotlist", NULL, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    LONGS_EQUAL(2, infolist_integer (infolist, "priority"));
    STRCMP_EQUAL("core", infolist_string (infolist, "plugin_name"));
    STRCMP_EQUAL("weechat", infolist_string (infolist, "buffer_name"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);

    gui_buffer_set (gui_buffers, "hotlist", "-1");

    /* hotlist (empty) */
    infolist = hook_infolist_get (NULL, "hotlist", NULL, NULL);
    CHECK(infolist);
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);
}

/*
 * Tests functions:
 *   plugin_api_infolist_key_cb
 */

TEST(PluginApiInfo, InfolistKeyCb)
{
    struct t_infolist *infolist;

    /* invalid key context */
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "key", NULL, "invalid_context"));

    /* keys */
    infolist = hook_infolist_get (NULL, "key", NULL, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("backspace", infolist_string (infolist, "key"));
    CHECK(infolist_next (infolist));
    infolist_free (infolist);

    /* keys of context "search" */
    infolist = hook_infolist_get (NULL, "key", NULL, "search");
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("ctrl-q", infolist_string (infolist, "key"));
    CHECK(infolist_next (infolist));
    infolist_free (infolist);
}

/*
 * Tests functions:
 *   plugin_api_infolist_layout_cb
 */

TEST(PluginApiInfo, InfolistLayoutCb)
{
    struct t_infolist *infolist;
    struct t_gui_layout *ptr_layout;

    ptr_layout = gui_layout_alloc ("test_layout");
    CHECK(ptr_layout);
    gui_layout_add (ptr_layout);
    gui_layout_window_store (ptr_layout);

    /* layouts */
    infolist = hook_infolist_get (NULL, "layout", NULL, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("test_layout", infolist_string (infolist, "name"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);

    gui_layout_remove (ptr_layout);

    /* no layouts */
    infolist = hook_infolist_get (NULL, "layout", NULL, NULL);
    CHECK(infolist);
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);
}

/*
 * Tests functions:
 *   plugin_api_infolist_nicklist_cb
 */

TEST(PluginApiInfo, InfolistNicklistCb)
{
    struct t_infolist *infolist;

    /* missing buffer pointer */
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "nicklist", NULL, NULL));

    /* invalid buffer pointer */
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "nicklist", (void *)0x1, NULL));

    /* nicklist */
    infolist = hook_infolist_get (NULL, "nicklist", gui_buffers, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("group", infolist_string (infolist, "type"));
    STRCMP_EQUAL("root", infolist_string (infolist, "name"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);
}

/*
 * Tests functions:
 *   plugin_api_infolist_option_cb
 */

TEST(PluginApiInfo, InfolistOptionCb)
{
    struct t_infolist *infolist;

    /* invalid option name */
    infolist = hook_infolist_get (NULL, "option", NULL, "invalid.name");
    CHECK(infolist);
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);

    /* all options */
    infolist = hook_infolist_get (NULL, "option", NULL, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("alias", infolist_string (infolist, "config_name"));
    CHECK(infolist_next (infolist));
    infolist_free (infolist);

    /* option with name */
    infolist = hook_infolist_get (NULL, "option", NULL, "weechat.look.mouse");
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("weechat", infolist_string (infolist, "config_name"));
    STRCMP_EQUAL("weechat.look.mouse", infolist_string (infolist, "full_name"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);
}

/*
 * Tests functions:
 *   plugin_api_infolist_plugin_cb
 */

TEST(PluginApiInfo, InfolistPluginCb)
{
    struct t_infolist *infolist;
    struct t_weechat_plugin *ptr_plugin;
    const char *ptr_name;
    char *name;

    /* invalid plugin pointer */
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "plugin", (void *)0x1, NULL));

    /* all plugins */
    infolist = hook_infolist_get (NULL, "plugin", NULL, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    ptr_plugin = (struct t_weechat_plugin *)infolist_pointer (infolist, "pointer");
    CHECK(infolist_integer (infolist, "priority") > 0);
    ptr_name = infolist_string (infolist, "name");
    CHECK(ptr_name);
    name = strdup (ptr_name);
    CHECK(infolist_next (infolist));
    infolist_free (infolist);

    /* one plugin with pointer */
    infolist = hook_infolist_get (NULL, "plugin", ptr_plugin, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL(name, infolist_string (infolist, "name"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);

    /* one plugin with name */
    infolist = hook_infolist_get (NULL, "plugin", NULL, "spel*");
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("spell", infolist_string (infolist, "name"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);

    free (name);
}

/*
 * Tests functions:
 *   plugin_api_infolist_proxy_cb
 */

TEST(PluginApiInfo, InfolistProxyCb)
{
    struct t_infolist *infolist;
    struct t_proxy *ptr_proxy1, *ptr_proxy2;

    /* invalid proxy pointer */
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "proxy", (void *)0x1, NULL));

    ptr_proxy1 = proxy_new ("proxy1", "http", "off",
                            "proxy1.example.com", "8000", "user1", "pass1");
    CHECK(ptr_proxy1);
    ptr_proxy2 = proxy_new ("proxy2", "http", "off",
                            "proxy2.example.com", "9000", "user2", "pass2");
    CHECK(ptr_proxy2);

    /* all proxies */
    infolist = hook_infolist_get (NULL, "proxy", NULL, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("proxy1", infolist_string (infolist, "name"));
    CHECK(infolist_next (infolist));
    infolist_free (infolist);

    /* one proxy with pointer */
    infolist = hook_infolist_get (NULL, "proxy", ptr_proxy2, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("proxy2", infolist_string (infolist, "name"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);

    /* one proxy with name (mask) */
    infolist = hook_infolist_get (NULL, "proxy", NULL, "*xy1");
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("proxy1", infolist_string (infolist, "name"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);

    proxy_free (ptr_proxy1);
    proxy_free (ptr_proxy2);
}

/*
 * Tests functions:
 *   plugin_api_infolist_url_options_cb
 */

TEST(PluginApiInfo, InfolistUrlOptionsCb)
{
    struct t_infolist *infolist;

    /* URL options */
    infolist = hook_infolist_get (NULL, "url_options", NULL, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("VERBOSE", infolist_string (infolist, "name"));
    CHECK(infolist_next (infolist));
    infolist_free (infolist);
}

/*
 * Tests functions:
 *   plugin_api_infolist_window_cb
 */

TEST(PluginApiInfo, InfolistWindowCb)
{
    struct t_infolist *infolist;

    /* invalid window pointer */
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "window", (void *)0x1, NULL));

    /* invalid window number */
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "window", NULL, "123"));

    /* all windows */
    infolist = hook_infolist_get (NULL, "window", NULL, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    POINTERS_EQUAL(gui_buffers, infolist_pointer (infolist, "buffer"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);

    /* one window with pointer */
    infolist = hook_infolist_get (NULL, "window", gui_windows, NULL);
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    POINTERS_EQUAL(gui_buffers, infolist_pointer (infolist, "buffer"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);

    /* current window */
    infolist = hook_infolist_get (NULL, "window", NULL, "current");
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    POINTERS_EQUAL(gui_buffers, infolist_pointer (infolist, "buffer"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);

    /* one window with number */
    infolist = hook_infolist_get (NULL, "window", NULL, "1");
    CHECK(infolist);
    CHECK(infolist_next (infolist));
    POINTERS_EQUAL(gui_buffers, infolist_pointer (infolist, "buffer"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    infolist_free (infolist);
}

/*
 * Tests functions:
 *   plugin_api_info_init
 */

TEST(PluginApiInfo, Init)
{
    /* TODO: write tests */
}
