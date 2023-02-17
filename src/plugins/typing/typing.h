/*
 * Copyright (C) 2021-2023 Sébastien Helleu <flashcode@flashtux.org>
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

extern void typing_setup_hooks ();

#endif /* WEECHAT_PLUGIN_TYPING_H */
