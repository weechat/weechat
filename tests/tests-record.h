/*
 * Copyright (C) 2023-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_TESTS_RECORD_H
#define WEECHAT_TESTS_RECORD_H

#define RECORD_CHECK_NO_MSG()                                           \
    if (record_count_messages () > 0)                                   \
    {                                                                   \
        char **msg = string_dyn_alloc (256);                            \
        string_dyn_concat (                                             \
            msg,                                                        \
            "Unexpected message(s) displayed:\n: ", -1);                \
        record_dump (msg);                                              \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

#define RECORD_CHECK_MSG(__buffer, __prefix, __message, __tags)         \
    if (!record_search (__buffer, __prefix, __message, __tags))         \
    {                                                                   \
        char **msg = string_dyn_alloc (256);                            \
        string_dyn_concat (msg, "Message not displayed: ", -1);         \
        string_dyn_concat (msg, "buffer=\"", -1);                       \
        string_dyn_concat (msg, __buffer, -1);                          \
        string_dyn_concat (msg, "\", prefix=\"", -1);                   \
        string_dyn_concat (msg, __prefix, -1);                          \
        string_dyn_concat (msg, "\", message=\"", -1);                  \
        string_dyn_concat (msg, __message, -1);                         \
        string_dyn_concat (msg, "\", tags=\"", -1);                     \
        string_dyn_concat (msg, __tags, -1);                            \
        string_dyn_concat (msg, "\"\n", -1);                            \
        string_dyn_concat (msg, "All messages displayed:\n", -1);       \
        record_dump (msg);                                              \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

extern struct t_arraylist *recorded_messages;

extern void record_start ();
extern void record_stop ();
extern struct t_hashtable *record_search (const char *buffer, const char *prefix,
                                          const char *message, const char *tags);
extern int record_count_messages ();
extern void record_dump (char **msg);
extern void record_error_missing (const char *message);

#endif /* WEECHAT_TESTS_RECORD_H */
