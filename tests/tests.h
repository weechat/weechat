/*
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

#ifndef WEECHAT_TESTS_H
#define WEECHAT_TESTS_H

#define WEE_TEST_STR(__result, __test)                                  \
    str = __test;                                                       \
    if (__result == NULL)                                               \
    {                                                                   \
        POINTERS_EQUAL(NULL, str);                                      \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__result, str);                                    \
    }                                                                   \
    free (str);

extern void run_cmd (const char *command);
extern void run_cmd_quiet (const char *command);

#endif /* WEECHAT_TESTS_H */
