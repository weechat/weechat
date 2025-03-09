/*
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

#ifndef WEECHAT_SIGNAL_H
#define WEECHAT_SIGNAL_H

struct t_signal
{
    int signal;                        /* signal number                     */
    char *name;                        /* signal name, eg "hup" for SIGHUP  */
};

extern struct t_signal signal_list[];

extern int signal_search_number (int signal_number);
extern int signal_search_name (const char *name);
extern void signal_catch (int signum, void (*handler)(int));
extern void signal_handle (void);
extern void signal_suspend (void);
extern void signal_init (void);

#endif /* WEECHAT_SIGNAL_H */
