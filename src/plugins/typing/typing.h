/*
 * SPDX-FileCopyrightText: 2021-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_TYPING_H
#define WEECHAT_PLUGIN_TYPING_H

#define weechat_plugin weechat_typing_plugin
#define TYPING_PLUGIN_NAME "typing"
#define TYPING_PLUGIN_PRIORITY 8000

struct t_typing
{
    struct t_hook *hook;               /* command hook                      */
    char *name;                        /* typing name                       */
    char *command;                     /* typing command                    */
    char *completion;                  /* completion for typing (if not set,*/
                                       /* uses completion of target cmd)    */
    int running;                       /* 1 if typing is running            */
    struct t_typing *prev_typing;      /* link to previous typing           */
    struct t_typing *next_typing;      /* link to next typing               */
};

extern struct t_typing *typing_list;

extern struct t_weechat_plugin *weechat_typing_plugin;

extern void typing_setup_hooks (void);

#endif /* WEECHAT_PLUGIN_TYPING_H */
