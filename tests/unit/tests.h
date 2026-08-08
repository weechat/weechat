/*
 * SPDX-FileCopyrightText: 2014-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_TESTS_H
#define WEECHAT_TESTS_H

#define WEE_TEST_STR(__result, __test)                                  \
    str = __test;                                                       \
    STRCMP_EQUAL(__result, str);                                        \
    free (str);

extern void run_cmd (const char *command);
extern void run_cmd_quiet (const char *command);

#endif /* WEECHAT_TESTS_H */
