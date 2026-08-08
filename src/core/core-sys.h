/*
 * SPDX-FileCopyrightText: 2023-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
