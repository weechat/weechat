/*
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_ALIAS_H
#define WEECHAT_PLUGIN_ALIAS_H

#define weechat_plugin weechat_alias_plugin
#define ALIAS_PLUGIN_NAME "alias"
#define ALIAS_PLUGIN_PRIORITY 11000

struct t_alias
{
    struct t_hook *hook;               /* command hook                      */
    char *name;                        /* alias name                        */
    char *command;                     /* alias command                     */
    char *completion;                  /* completion for alias (if not set, */
                                       /* uses completion of target cmd)    */
    int running;                       /* 1 if alias is running             */
    struct t_alias *prev_alias;        /* link to previous alias            */
    struct t_alias *next_alias;        /* link to next alias                */
};

extern struct t_alias *alias_list;

extern struct t_weechat_plugin *weechat_alias_plugin;

extern int alias_valid (struct t_alias *alias);
extern struct t_alias *alias_search (const char *alias_name);
extern void alias_update_completion (struct t_alias *alias,
                                     const char *completion);
extern struct t_alias *alias_new (const char *name, const char *command,
                                  const char *completion);
extern int alias_rename (struct t_alias *alias, const char *new_name);
extern void alias_free (struct t_alias *alias);
extern void alias_free_all ();
extern int alias_add_to_infolist (struct t_infolist *infolist,
                                  struct t_alias *alias);

#endif /* WEECHAT_PLUGIN_ALIAS_H */
