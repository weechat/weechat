/*
 * Copyright (C) 2010-2013 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WEECHAT_RMODIFIER_H
#define __WEECHAT_RMODIFIER_H 1

#include <regex.h>

#define weechat_plugin weechat_rmodifier_plugin
#define RMODIFIER_PLUGIN_NAME "rmodifier"

struct t_rmodifier
{
    char *name;                         /* name of rmodifier                */
    char *modifiers;                    /* modifiers                        */
    struct t_hook **hooks;              /* hooks for modifiers              */
    int hooks_count;                    /* number of hooks                  */
    char *str_regex;                    /* string with regex                */
    regex_t *regex;                     /* regex                            */
    char *groups;                       /* actions on groups in regex       */
                                        /* (keep, delete, hide)             */
    struct t_rmodifier *prev_rmodifier; /* link to previous rmodifier       */
    struct t_rmodifier *next_rmodifier; /* link to next rmodifier           */
};

extern struct t_rmodifier *rmodifier_list;
extern int rmodifier_count;

extern struct t_weechat_plugin *weechat_rmodifier_plugin;

extern int rmodifier_valid (struct t_rmodifier *rmodifier);
extern struct t_rmodifier *rmodifier_search (const char *name);
struct t_rmodifier *rmodifier_new (const char *name,
                                   const char *modifiers,
                                   const char *str_regex,
                                   const char *groups);
extern struct t_rmodifier *rmodifier_new_with_string (const char *name,
                                                      const char *value);
extern void rmodifier_create_default ();
extern void rmodifier_free (struct t_rmodifier *rmodifier);
extern void rmodifier_free_all ();
extern int rmodifier_add_to_infolist (struct t_infolist *infolist,
                                      struct t_rmodifier *rmodifier);
extern void rmodifier_print_log ();

#endif /* __WEECHAT_RMODIFIER_H */
