/*
 * SPDX-FileCopyrightText: 2023-2025 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

#ifndef WEECHAT_SYS_H
#define WEECHAT_SYS_H

#ifdef HAVE_SYS_RESOURCE_H
struct t_rlimit_resource
{
    char *name;                        /* name of resource                  */
    int resource;                      /* value of resource                 */
};
#endif /* HAVE_SYS_RESOURCE_H */

extern void sys_setrlimit (void);
extern void sys_display_rlimit (void);
extern void sys_display_rusage (void);
extern void sys_waitpid (int number_processes);

#endif /* WEECHAT_SYS_H */
