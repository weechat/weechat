/*
 * Copyright (C) 2023 Sébastien Helleu <flashcode@flashtux.org>
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

extern struct t_arraylist *recorded_messages;

extern void record_start ();
extern void record_stop ();
extern int record_search (const char *buffer, const char *prefix,
                          const char *message, const char *tags);
extern void record_dump (char **msg);
extern void record_error_missing (const char *message);

#endif /* WEECHAT_TESTS_RECORD_H */
