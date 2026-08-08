/*
 * SPDX-FileCopyrightText: 2021-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
